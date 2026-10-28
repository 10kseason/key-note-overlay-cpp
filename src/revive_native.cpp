#include "revive_native.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <tuple>
namespace revive {
using V=std::vector<double>;
constexpr double nil=std::numeric_limits<double>::quiet_NaN();
double clamp(double x){return std::clamp(x,0.0,1.0);}
double roundn(double x,int n=9){double s=std::pow(10.,n);return std::nearbyint(x*s)/s;}
double pct(V v,double p){if(v.empty())return 0;std::sort(v.begin(),v.end());double x=(v.size()-1)*p/100.;int lo=int(x),hi=int(std::ceil(x));return v[lo]+(v[hi]-v[lo])*(x-lo);}
struct Budget {
    const std::atomic<bool>* cancelled;
    std::chrono::steady_clock::time_point deadline;
    void Check() const {
        if (cancelled && cancelled->load()) throw std::runtime_error("revive cancelled");
        if (std::chrono::steady_clock::now() >= deadline) throw std::runtime_error("revive time limit");
    }
};
// The two judgment routines intentionally differ in null semantics, as in Python.
std::array<double,2> direct(double ms,int type,const Config& c){
    if(c.judgments.empty())return {100,100};
    for(auto&r:c.judgments){double bound=r[type*2+(ms>=0?0:1)];if(std::isnan(bound)||std::abs(ms)<=std::abs(bound))return {r[6],r[7]};}
    auto&r=c.judgments.back();return {r[6],r[7]};
}
std::array<double,2> typed(double ms,int type,const Config& c){
    if(c.judgments.empty())return {100,100};
    auto lookup=[&](bool full){for(auto&r:c.judgments){double p=r[type*2],m=r[type*2+1];if(std::isnan(p)||(full&&std::isnan(m))||ms<=std::abs(p)+(full?std::abs(m):0))return std::array<double,2>{std::isnan(r[6])?0:r[6],std::isnan(r[7])?0:r[7]};}return std::array<double,2>{nil,nil};};
    auto a=lookup(false),b=lookup(true);if(std::isnan(a[0]))return b;if(std::isnan(b[0]))return a;return {(a[0]+b[0])/2,(a[1]+b[1])/2};
}
using Deltas=std::array<std::array<std::array<double,2>,3>,2>; // score/acc, type, +/- seconds
Deltas deltas(const Config& c){
    Deltas d{};
    for(int type=0;type<3;type++)for(int sign=0;sign<2;sign++){
        int score=0,acc95=0;bool found=false;
        for(auto&r:c.judgments){double b=r[type*2+sign];if(std::isnan(b))continue;int v=int(sign?std::abs(b):b);if(r[6]==100)score=std::max(score,v);if(r[7]>=95){found=true;acc95=std::max(acc95,v);}}
        int acc=score;if(found&&c.timeDelta=='A')acc=std::max(score,std::max(0,acc95-1));if(found&&c.timeDelta=='B')acc=int(std::max(double(score),(score+acc95)*.5));
        d[0][type][sign]=score/1000.;d[1][type][sign]=acc/1000.;
    }return d;
}
struct Acc {double n=0,d=0;void add(double ratio,double w){if(!std::isnan(ratio)){n+=ratio*w;d+=w;}}double value()const{return d>0?n/d:100.;}};
struct State {std::array<double,MaxKeys> offsets;double length=0,space=0;std::array<Acc,4> previous;State(){offsets.fill(nil);}};
static const std::vector<std::string> names={"nps","nps_v2","distance_difficulty","same_line_nps_v2","minimum_distance_sum","same_line_minimum_distance_sum","spider_distance_nerf","j75","j100","j125","j150","jack_nps_v2","jack_interval","jack_score_uniformity","jack_acc_uniformity","note_jack_diff_score","note_jack_diff_acc","fds","fds_d","fda","fda_d","rds","rds_d","rda","rda_d","lfds","lfds_d","lfda","lfda_d","lrds","lrds_d","lrda","lrda_d","ldb","ldbd","vrs","vra","note_score_diff","note_acc_diff"};
void radar(const std::vector<Note>&,double,const Config&,Result&,const Budget&);
void spider(const std::vector<Note>&,double,const Config&,Result&,const Budget&);
Result calculate(std::vector<Note> notes,double duration,const Config& c,
    const std::atomic<bool>* cancelled, std::chrono::milliseconds timeout){
    const Budget budget{cancelled, std::chrono::steady_clock::now() + timeout};
    budget.Check();
    if (!SupportsKeys(c.keys) || notes.size() > MaxEvents || !std::isfinite(duration) || duration <= 0)
        throw std::runtime_error("revive input limit");
    for (const auto& note : notes)
        if (!std::isfinite(note.time) || note.time < 0 || note.time > 7200 ||
            note.lane < 0 || note.lane >= c.keys || note.type < 0 || note.type > 2)
            throw std::runtime_error("revive invalid input");
    auto order=[](int t){return t==2?0:t==0?1:2;};
    std::stable_sort(notes.begin(),notes.end(),[&](auto&a,auto&b){return std::make_tuple(a.time,order(a.type),a.lane)<std::make_tuple(b.time,order(b.type),b.lane);});
    // Bound each neighbourhood before allocating per-note results. A corrupt
    // dense chart must not make cancellation wait inside a quadratic scan.
    for (std::size_t left = 0, right = 0; right < notes.size(); ++right) {
        while (notes[left].time < notes[right].time - 2.0) ++left;
        if (right - left + 1 > 2048) throw std::runtime_error("revive density limit");
    }
    budget.Check();
    int n=int(notes.size());Result out;for(auto&name:names)out.arrays[name]=V(n);
    auto&a=out.arrays;auto&s=out.scalars;
    // Resolve result-array names once; keep string lookup out of note-pair loops.
    auto& v_distance_difficulty=a["distance_difficulty"];
    auto& v_jack_acc_uniformity=a["jack_acc_uniformity"];
    auto& v_jack_interval=a["jack_interval"];
    auto& v_jack_nps_v2=a["jack_nps_v2"];
    auto& v_jack_score_uniformity=a["jack_score_uniformity"];
    auto& v_ldb=a["ldb"];
    auto& v_ldbd=a["ldbd"];
    auto& v_minimum_distance_sum=a["minimum_distance_sum"];
    auto& v_note_jack_diff_acc=a["note_jack_diff_acc"];
    auto& v_note_jack_diff_score=a["note_jack_diff_score"];
    auto& v_nps=a["nps"];
    auto& v_nps_v2=a["nps_v2"];
    auto& v_same_line_minimum_distance_sum=a["same_line_minimum_distance_sum"];
    auto& v_same_line_nps_v2=a["same_line_nps_v2"];
    auto& v_spider_distance_nerf=a["spider_distance_nerf"];
    auto& v_vra=a["vra"];
    auto& v_vrs=a["vrs"];

    const double window=c.get("weighted_nps.time_window_ms",1000), ws=window/1000.;const auto td=deltas(c);
    std::array<std::vector<int>,MaxKeys> lines;std::array<V,MaxKeys> lineTimes;std::vector<int> pos(n);
    for(int i=0;i<n;i++){auto&v=lines[notes[i].lane];pos[i]=int(v.size());v.push_back(i);lineTimes[notes[i].lane].push_back(notes[i].time);}
    V tails(n,1),nerfs(n);std::vector<int> starts(n),finishes(n);
    for(int i=0;i<n;i++)if(notes[i].type==2&&pos[i]>0)tails[i]=std::clamp((notes[i].time-notes[lines[notes[i].lane][pos[i]-1]].time)*1000/window,.05,1.);
    auto&nv=v_nps_v2;auto&same=v_same_line_nps_v2;auto&dist=v_distance_difficulty;auto&jack=v_jack_nps_v2;auto&ji=v_jack_interval;auto&ju=v_jack_score_uniformity;auto&jua=v_jack_acc_uniformity;
    int left=0,right=0;double previous=nil;
    const double threshold=c.get("jack_difficulty.vibro_nerf_nps_condition",10),nerfWeight=c.get("jack_difficulty.vibro_nps_nerf_weight",.9);
    for(int i=0;i<n;i++){
        budget.Check();
        auto note=notes[i];double t=note.time;int col=note.lane;std::vector<int> full,half;
        if(std::isnan(previous)||t!=previous){while(left<right&&notes[left].time<=t-ws)left++;while(right<n&&notes[right].time<t+ws)right++;previous=t;}
        starts[i]=left;finishes[i]=right;double pdelta=window,mdelta=window;
        for(int j=left;j<right;j++){
            double dt=(t-notes[j].time)*1000,ab=std::abs(dt);if(-500<dt&&dt<=500)v_nps[i]++;
            if(ab>=window)continue;
            double tw=1-ab/window;double weight=c.weights[col][notes[j].lane]*(j!=i?tails[j]:1)*tw;nv[i]+=weight;
            if(col==notes[j].lane){same[i]+=weight;if(notes[j].type!=2){full.push_back(j);if(ab<window*.5)half.push_back(j);jack[i]+=tw;}}
            if(dt>0)pdelta=std::min(pdelta,dt);else if(dt<0)mdelta=std::min(mdelta,std::abs(dt));
        }
        auto uniformity=[&](const std::vector<int>&ids,bool halfWindow,double& interval){
            std::array<double,2> sums{};interval=std::max(0.,(notes[ids.back()].time-notes[ids.front()].time)/(ids.size()-1));
            for(int k=0;k<int(ids.size());k++){
                auto target=notes[ids[k]];int type=target.type==1?1:0;double timing=notes[ids.front()].time+k*interval;
                for(int kind=0;kind<2;kind++)for(double offset:{0.,td[kind][type][0],-td[kind][type][1]}){
                    double r=typed(std::abs((target.time-(timing+offset))*1000),type,c)[kind];if(!std::isnan(r))sums[kind]+=r;
                }
            }
            for(auto&v:sums)v=std::clamp(v/(halfWindow?3.:6.)-ids.size()*(halfWindow?100.:50.)+100.,0.,100.);
            return sums;
        };
        double interval=ws,halfInterval=0;std::array<double,2> u{100,100},h{100,100};
        if(full.size()>=2){if(!c.judgments.empty())u=uniformity(full,false,interval);else interval=std::max(0.,(notes[full.back()].time-notes[full.front()].time)/(full.size()-1));}
        if(!c.judgments.empty()&&half.size()>=3){h=uniformity(half,true,halfInterval);if(u[0]<h[0])interval=halfInterval;}
        // Preserve source behavior: an uncomputed half-window defaults to 100.
        ju[i]=std::max(u[0],h[0]);jua[i]=std::max(u[1],h[1]);ji[i]=interval*1000;
        if(!c.judgments.empty()&&jack[i]>threshold&&jack[i]>0)nerfs[i]=clamp((jack[i]-threshold)/jack[i]*clamp(ju[i]/100))*nerfWeight;
        same[i]*=1-nerfs[i];nv[i]*=1-nerfs[i];double minimum=std::max(std::min(pdelta,mdelta),10.);
        for(int j=left;j<right;j++)if(j!=i){double dt=std::abs((t-notes[j].time)*1000);double distance=c.visual[col][notes[j].lane]+c.types[note.type][notes[j].type]+std::min(dt/minimum,3.)+dt/window;dist[i]=dist[i]==0?distance:std::min(dist[i],distance);}
        dist[i]=std::max(1.,dist[i]);
    }
    for(int i=0;i<n;i++){
        budget.Check();
        for(int j=starts[i];j<finishes[i];j++){double tw=1-std::abs((notes[i].time-notes[j].time)*1000)/window;double v=dist[j]*(i!=j?tails[j]:1)*tw;v_minimum_distance_sum[i]+=v;if(notes[i].lane==notes[j].lane)v_same_line_minimum_distance_sum[i]+=v;}
        v_minimum_distance_sum[i]*=1-nerfs[i];v_same_line_minimum_distance_sum[i]*=1-nerfs[i];
    }
    spider(notes,window,c,out,budget);
    std::array<double,4> base{75,100,125,150},coefs,jn;
    std::array<std::string,4> jnames{"j75","j100","j125","j150"};
    std::array<V*,4> jarrays;
    for(int k=0;k<4;k++)jarrays[k]=&a[jnames[k]];
    for(int k=0;k<4;k++){coefs[k]=c.get("jack_difficulty.coefficients."+jnames[k],std::array<double,4>{1,1,.9,.1}[k]);jn[k]=c.get("jack_difficulty.vibro_"+jnames[k]+"_nerf_weight",std::array<double,4>{.6,.9,.5,.5}[k]);}
    std::array<int,MaxKeys> last;last.fill(-1);std::array<std::array<double,4>,MaxKeys> accum{};std::vector<State> states(n);
    const bool lnHalf=c.get("jack_difficulty.ln_half_interval",1)!=0;
    const std::array<std::string,8> fn{"fds","fda","rds","rda","lfds","lfda","lrds","lrda"};
    std::array<V*,8> farrays,denominators;
    for(int k=0;k<8;k++){farrays[k]=&a[fn[k]];denominators[k]=&a[fn[k]+"_d"];}
    for(int i=0;i<n;i++){
        budget.Check();
        auto note=notes[i];double t=note.time;int col=note.lane;int prev=last[col];
        if(prev>=0){double actual=t*1000-notes[prev].time*1000;for(int k=0;k<4;k++){double interval=base[k]*(lnHalf&&notes[prev].type!=0?.5:1.);accum[col][k]=std::max(0.,accum[col][k]+interval-actual);(*jarrays[k])[i]=roundn(accum[col][k]);}}
        last[col]=i;
        for(int k=0;k<4;k++)if((*jarrays[k])[i]>0){auto ratio=typed((*jarrays[k])[i],note.type,c);if(!std::isnan(ratio[0]))for(int kind=0;kind<2;kind++){double coef=coefs[k];if(!c.judgments.empty())coef=std::max(0.,coef-coef*clamp((kind?jua[i]:ju[i])/100)*jn[k]);(kind?v_note_jack_diff_acc:v_note_jack_diff_score)[i]+=coef*(100-ratio[kind]);}}
        v_note_jack_diff_score[i]=roundn(v_note_jack_diff_score[i]);v_note_jack_diff_acc[i]=roundn(v_note_jack_diff_acc[i]);
        int next=pos[i]+1<int(lines[col].size())?lines[col][pos[i]+1]:-1;
        State state;if(prev>=0){state=states[prev];state.space=std::min(state.space,.075);if(note.type==2){state.length=t-notes[prev].time;if(next>=0)state.space=std::min(std::max(0.,notes[next].time-t),.075);}}
        std::array<Acc,8> sums;for(auto&v:sums)v={300*c.weights[col][col],3*c.weights[col][col]};
        if(note.type==2&&prev>=0)for(int k=0;k<4;k++)sums[k]=states[prev].previous[k];
        std::array<Acc,2> relation;double minLength=0,minSpace=0,ldb=0,ldbd=0;
        for(int direction:{-1,1}){
            double press=t,release=t;
            for(int lane=col+direction;lane>=0&&lane<c.keys;lane+=direction){
                auto&ln=lines[lane];if(ln.empty())continue;int p=int(std::lower_bound(lineTimes[lane].begin(),lineTimes[lane].end(),t)-lineTimes[lane].begin());
                int selected=-1,tail=-1,head=-1,prior=-1;
                if(p<int(ln.size())){int cand=ln[p];if(notes[cand].type==2){tail=cand;if(p>0)head=ln[p-1];if(p+1<int(ln.size())&&notes[ln[p+1]].type!=2)selected=ln[p+1];}else selected=cand;}
                if(p>0){int cand=ln[p-1];if(notes[cand].type==2){if(p>=2)prior=ln[p-2];}else prior=cand;}
                if(selected<0&&tail<0&&prior<0)continue;
                double w=c.weights[col][lane];
                int closest=-1;if(p<int(ln.size())&&p>0)closest=std::abs(notes[ln[p]].time-t)<std::abs(t-notes[ln[p-1]].time)?ln[p]:ln[p-1];else if(p<int(ln.size()))closest=ln[p];else if(p>0)closest=ln[p-1];
                if(closest>=0){double ci=ji[i]>0?ji[i]:window,cj=ji[closest]>0?ji[closest]:window;double rel=std::max(1-std::abs(window/ci-window/cj),0.);double tw=std::max((window-std::abs(t-notes[closest].time)*1000)/window,0.);relation[0].add(ju[i]/100*ju[closest]/100*rel,w*tw);relation[1].add(jua[i]/100*jua[closest]/100*rel,w*tw);}
                auto typeOf=[&](int idx){return idx>=0&&notes[idx].type==1?1:0;};
                auto ratioAt=[&](double pt,int idx,double delta,int kind){return idx<0?nil:direct(((pt+delta)-notes[idx].time)*1000,typeOf(idx),c)[kind];};
                auto pair=[&](Acc&sum,double pt,int kind){for(int sign=0;sign<3;sign++){
                    auto delta=[&](int idx){return sign==0?0.:(sign==1?td[kind][typeOf(idx)][0]:-td[kind][typeOf(idx)][1]);};
                    double r=ratioAt(pt,selected,delta(selected),kind),pr=ratioAt(pt,prior,delta(prior),kind);
                    if(!std::isnan(r)&&!std::isnan(pr)&&pr>0)r=std::max(r,pr);else if(std::isnan(r)&&!std::isnan(pr)&&pr>0)r=pr;
                    sum.add(r,w);
                }};
                if(note.type!=2&&(selected>=0||prior>=0)){for(int kind=0;kind<2;kind++){pair(sums[kind],press,kind);if(!std::isnan(state.offsets[lane]))pair(sums[2+kind],t+state.offsets[lane],kind);}}
                if(note.type!=2&&(selected>=0||prior>=0)){
                    double r=ratioAt(t,selected,td[0][typeOf(selected)][0],0),pr=ratioAt(t,prior,-td[0][typeOf(prior)][1],0);int chosen=-1;
                    if(!std::isnan(r)&&!std::isnan(pr)&&pr>0)chosen=notes[selected].time-t<t-notes[prior].time?selected:prior;else if(!std::isnan(r))chosen=selected;else if(!std::isnan(pr)&&pr>0)chosen=prior;
                    if(chosen<0)state.offsets[lane]=nil;else {press=notes[chosen].time;state.offsets[lane]=press-t;}
                }
                if(tail>=0&&head>=0){
                    double st=notes[tail].time,pt=notes[head].time,nt=selected>=0?notes[selected].time:nil;double ld=std::min(t-pt,st-t);if(ld>0){ldb+=w*std::min(ld,1.);ldbd+=std::min(ld,1.);}
                    for(int kind=0;kind<2;kind++){
                        auto average=[&](int type,double target){double sum=0;for(double delta:{0.,td[kind][type][0],-td[kind][type][1]}){double r=direct(((t+delta)-target)*1000,type,c)[kind];if(!std::isnan(r))sum+=r;}return sum/3;};
                        double lw=(100-std::max(average(2,st),average(1,pt)))/100;
                        if(lw>0){for(double delta:{0.,td[kind][2][0],-td[kind][2][1]}){
                            sums[4+kind].add(direct(((release+delta)-st)*1000,2,c)[kind],w*lw);
                            double r=direct((((pt+state.length)+delta)-st)*1000,2,c)[kind];
                            if(!std::isnan(nt)){double other=direct((((nt-state.space)+delta)-st)*1000,2,c)[kind];if(std::isnan(r))r=other;else if(!std::isnan(other))r=std::max(r,other);}
                            sums[6+kind].add(r,w*lw);
                        }}
                    }
                    if(!std::isnan(direct(((t+td[0][2][0])-st)*1000,2,c)[0]))release=st;
                    double len=st-pt;if(len>0)minLength=minLength<=0?len:std::min(minLength,len);
                    if(!std::isnan(nt)){double space=nt-st;if(space>0)minSpace=minSpace<=0?space:std::min(minSpace,space);}
                }
            }
        }
        if(minLength>0)state.length=minLength;
        if(minSpace>0)state.space=std::min(minSpace,.075);
        for(int k=0;k<4;k++)state.previous[k]=sums[k];
        states[i]=state;
        for(int k=0;k<8;k++){(*farrays[k])[i]=roundn(sums[k].value());(*denominators[k])[i]=roundn(sums[k].d);}
        v_vrs[i]=roundn(relation[0].d>0?relation[0].n/relation[0].d:1);v_vra[i]=roundn(relation[1].d>0?relation[1].n/relation[1].d:1);
        v_ldb[i]=roundn(ldb);v_ldbd[i]=roundn(ldbd);
    }
    // Keep the original per-call judgment integration; no preset caching in this benchmark.
    std::array<std::array<double,2>,3> judge;
    for(int type=0;type<3;type++){std::array<Acc,2> sum;for(int i=0;i<int(window);i++)for(int offset:{i,-i}){auto r=direct(offset,type,c);for(int kind=0;kind<2;kind++)sum[kind].add(r[kind],1./(i+1));}for(int kind=0;kind<2;kind++)judge[type][kind]=sum[kind].value();}
    for(int type=0;type<3;type++)for(int kind=0;kind<2;kind++)s[std::string("judge_difficulty_")+(kind?"acc_":"score_")+std::array<std::string,3>{"rice","head","tail"}[type]]=roundn(judge[type][kind]);
    double dw=c.get("jack_difficulty.distance_weight",.5),ldbW=c.get("ldb_weight",1),vrW=c.get("vibro_relation_nerf_weight",c.get("jack_difficulty.vibro_relation_nerf_weight",.5));
    std::array<double,4> fw{c.get("jack_difficulty.fd_weight",1),c.get("jack_difficulty.lfd_weight",1),c.get("jack_difficulty.rd_weight",1),c.get("jack_difficulty.lrd_weight",1)};
    std::array<double,2> l1{},l5{};
    for(int i=0;i<n;i++)for(int kind=0;kind<2;kind++){
        if (kind == 0) budget.Check();
        double flex=(((100/std::max((*farrays[kind])[i],.01))-1)*fw[0])+(((100/std::max((*farrays[4+kind])[i],.01))-1)*fw[1])+(((100/std::max((*farrays[2+kind])[i],.01))-1)*fw[2])+(((100/std::max((*farrays[6+kind])[i],.01))-1)*fw[3])+1;
        double other=nv[i]-same[i]+ldbW*v_ldb[i],nr=clamp((kind?v_vra:v_vrs)[i]*std::max(0.,vrW));double sd=v_same_line_minimum_distance_sum[i],od=v_minimum_distance_sum[i]-sd+ldbW*v_ldbd[i];od=std::max(0.,od*(1-clamp(v_spider_distance_nerf[i])));
        double b=std::pow((same[i]+other*(1-nr))*std::pow(sd+od*(1-nr),dw),1/(1+dw))*(1+(kind?v_note_jack_diff_acc:v_note_jack_diff_score)[i]/100);
        double v=b*flex*(100/std::max(judge[notes[i].type][kind],.01));a[kind?"note_acc_diff":"note_score_diff"][i]=v;l1[kind]+=v;l5[kind]+=std::pow(v,5);
    }
    double sw=c.get("score_weight",.1),aw=c.get("acc_weight",.9),rw=c.get("rating_weight",.343),rp=c.get("rating_power",.5),arp=c.get("avg_rating_power",1),alp=c.get("avg_lv_power",1);
    for(int k=0;k<2;k++){l1[k]=n?l1[k]/n:0;l5[k]=l5[k]>0?std::pow(l5[k],.2):0;s[k?"acc_diff_l5_sum":"score_diff_l5_sum"]=roundn(l5[k]);s[k?"acc_diff_l5_avg":"score_diff_l5_avg"]=roundn(l1[k]);}
    double ratingBase=sw*l5[0]*std::pow(l1[0],arp)+aw*l5[1]*std::pow(l1[1],arp);
    double difficulty=c.get("revive_score_weight",sw)*l5[0]*std::pow(l1[0],alp)+c.get("revive_acc_weight",aw)*l5[1]*std::pow(l1[1],alp);
    s["circus_rating"]=roundn(ratingBase>0&&rw>0?std::pow(rw*ratingBase,rp):0);s["revive_difficulty"]=roundn(difficulty);
    double scale=c.get("revive_level_scale",.073),power=c.get("revive_level_power",.81),maxLevel=c.get("revive_max_level",25),powered=difficulty*scale>0?std::pow(difficulty*scale,power):0;
    s["revive_lv"]=difficulty>0?std::ceil(maxLevel>0?maxLevel-maxLevel*maxLevel/(powered+maxLevel):powered):0;
    s["global_nps"]=n/std::max(duration,1.);s["peak_nps"]=n?*std::max_element(v_nps.begin(),v_nps.end()):0;
    radar(notes,duration,c,out,budget);budget.Check();return out;
}
void spider(const std::vector<Note>&notes,double window,const Config&c,Result&out,const Budget&budget){
    auto g=[&](const char*k,double d){return c.get(std::string("spider_distance_nerf.")+k,d);};
    if(!g("enabled",0)||notes.empty())return;
    auto&a=out.arrays;int n=int(notes.size());double maxN=std::clamp(g("max_nerf",.22),0.,.75),ws=std::max(1.,g("window_ms",window))/1000;
    double low=std::max(1.,g("min_interval_ms",120)),high=std::max(low,g("max_interval_ms",190)),maxCV=std::max(0.,g("max_interval_cv",.28));
    int minL=int(std::max(1.,g("min_unique_lanes",4))),minN=int(std::max(2.,g("min_notes",6)));
    double minE=clamp(g("min_entropy_ratio",.70)),maxS=clamp(g("max_same_line_ratio",.55)),maxJ=clamp(g("max_jack_ratio",.35)),minD=clamp(g("min_other_distance_ratio",.55)),minDS=std::max(0.,g("min_distance_sum",16));
    int start=0,finish=0;
    for(int i=0;i<n;i++){
        budget.Check();
        double t=notes[i].time;while(start<n&&notes[start].time<t-ws)start++;while(finish<n&&notes[finish].time<=t+ws)finish++;
        int count=finish-start;if(count<minN)continue;std::array<int,MaxKeys> counts{};for(int j=start;j<finish;j++)counts[notes[j].lane]++;
        int unique=0;double entropy=0;for(int x:counts)if(x){unique++;double p=double(x)/count;entropy-=p*std::log(p);}if(unique<minL)continue;
        double er=entropy/std::log(std::max(2,unique));if(er<minE)continue;
        V distinct,intervals;for(int j=start;j<finish;j++)if(distinct.empty()||std::abs(notes[j].time-distinct.back())>.002)distinct.push_back(notes[j].time);
        for(int j=1;j<int(distinct.size());j++)if(distinct[j]>distinct[j-1])intervals.push_back((distinct[j]-distinct[j-1])*1000);
        if(intervals.size()<3)continue;
        double median=pct(intervals,50);if(median<low||median>high)continue;
        double mean=std::accumulate(intervals.begin(),intervals.end(),0.)/intervals.size();if(mean<=0)continue;double variance=0;for(double v:intervals)variance+=std::pow(v-mean,2);variance/=intervals.size();double cv=std::sqrt(variance)/mean;if(cv>maxCV)continue;
        double nv=a["nps_v2"][i],sr=nv>0?a["same_line_nps_v2"][i]/nv:1,jr=nv>0?a["jack_nps_v2"][i]/nv:1;if(sr>maxS||jr>maxJ)continue;
        double ds=a["minimum_distance_sum"][i],ss=a["same_line_minimum_distance_sum"][i];if(ds<minDS)continue;double dr=std::max(ds-ss,0.)/ds;if(dr<minD)continue;
        double center=(low+high)*.5,half=std::max((high-low)*.5,1.);
        double score=clamp(1-std::abs(median-center)/half)+(maxCV<=0?1:clamp((maxCV-cv)/maxCV))+(minE>=1?1:clamp((er-minE)/(1-minE)))+(maxS<=0?1:clamp((maxS-sr)/maxS))+(maxJ<=0?1:clamp((maxJ-jr)/maxJ))+(minD>=1?1:clamp((dr-minD)/(1-minD)));
        a["spider_distance_nerf"][i]=roundn(maxN*score/6);
    }
}
struct Group {double time,last;int size=0;std::set<int> lanes;};
std::vector<Group> groups(const std::vector<Note>&notes,double eps,int filter){
    std::vector<Group> result;for(auto note:notes){if(filter==0&&note.type==2)continue;if(filter==2&&note.type!=2)continue;
        if(result.empty()||std::abs(note.time-result.back().last)>eps/1000)result.push_back({note.time,note.time,0,{}});
        auto&g=result.back();g.last=note.time;g.size++;g.lanes.insert(note.lane);
    }return result;
}
void radar(const std::vector<Note>&notes,double duration,const Config&c,Result&out,const Budget&budget){
    budget.Check();
    auto&a=out.arrays;auto&s=out.scalars;std::array<std::string,6> axes{"STREAM","SPEED","JACK","CHORD","TECH","HOLD"};
    if(!c.get("skill_radar.enabled",1)){for(auto&axis:axes){s["skill_radar."+axis]=0;s["skill_radar_raw."+axis]=0;}return;}
    auto p=[&](const char*k,double pc){return pct(a[k],pc);};
    auto penalty=[&](std::initializer_list<const char*>keys){V v;for(auto k:keys)for(double x:a[k])if(x>0)v.push_back(std::max(0.,100/std::max(x,.01)-1));return pct(v,90);};
    std::array<double,6> raw{};raw[0]=p("nps_v2",65)*.45+p("nps_v2",85)*.35+p("note_score_diff",65)*.20;raw[1]=p("nps_v2",96)*.65+p("nps",98)*.35;
    raw[2]=p("note_jack_diff_score",95)*.45+p("note_jack_diff_acc",95)*.35+p("jack_nps_v2",95)*4+p("same_line_nps_v2",95)*1.5;
    double dur=std::max(duration,1.),eps=c.get("skill_radar.chord_eps_ms",2),alpha=c.get("skill_radar.chord_alpha",.7);
    auto gs=groups(notes,eps,0);int maxChord=0,repeated=0;std::set<int> prevPattern;double prevTime=nil,pressure=0;
    for(auto&g:gs)if(g.size>=2){pressure+=std::pow(g.size,alpha);maxChord=std::max(maxChord,g.size);double gap=(g.time-prevTime)*1000;if(g.lanes==prevPattern&&!std::isnan(prevTime)&&gap>=40&&gap<=500)repeated++;prevPattern=g.lanes;prevTime=g.time;}
    raw[3]=pressure/dur+(repeated/dur)*1.5+std::max(0,maxChord-1)*.7;
    s["skill_radar_components.CHORD.chord_density"]=pressure/dur;s["skill_radar_components.CHORD.same_chord_repeat_density"]=repeated/dur;s["skill_radar_components.CHORD.max_chord_size"]=maxChord;
    V adjusted;for(int i=0;i<int(notes.size());i++)adjusted.push_back(std::max(0.,a["minimum_distance_sum"][i]-a["same_line_minimum_distance_sum"][i])*(1-clamp(a["spider_distance_nerf"][i])));
    raw[4]=pct(adjusted,90)*.75+p("note_acc_diff",85)*.15+penalty({"fds","fda","rds","rda"})*6;s["skill_radar_components.TECH.adjusted_other_distance_p90"]=pct(adjusted,90);
    std::array<V,MaxKeys> activeStarts;std::vector<std::pair<double,double>> intervals;
    for(auto note:notes){if(note.type==1)activeStarts[note.lane].push_back(note.time);else if(note.type==2&&!activeStarts[note.lane].empty()){double start=activeStarts[note.lane].back();activeStarts[note.lane].pop_back();if(note.time>start)intervals.emplace_back(start,note.time);}}
    double holdRaw=0,avg=0,coverage=0,conflict=0,releaseDensity=0;int peak=0,releasePeak=0;
    if(!intervals.empty()){
        std::vector<std::pair<double,int>> events;V starts,ends;double holdTime=0;
        for(auto&iv:intervals){holdTime+=iv.second-iv.first;events.emplace_back(iv.first,1);events.emplace_back(iv.second,-1);starts.push_back(iv.first);ends.push_back(iv.second);}
        avg=holdTime/dur;coverage=holdTime/(dur*c.keys);std::sort(events.begin(),events.end());int active=0;for(auto&e:events){active+=e.second;peak=std::max(peak,active);}
        std::sort(starts.begin(),starts.end());std::sort(ends.begin(),ends.end());double units=0;
        for(auto note:notes)if(note.type!=2){auto activeLanes=(std::lower_bound(starts.begin(),starts.end(),note.time)-starts.begin())-(std::upper_bound(ends.begin(),ends.end(),note.time)-ends.begin());units+=activeLanes;}
        auto releases=groups(notes,c.get("skill_radar.release_eps_ms",eps),2);int total=0;for(auto&g:releases){total+=g.size;releasePeak=std::max(releasePeak,g.size);}releaseDensity=total/dur;conflict=units/dur;
        holdRaw=avg*1.6+coverage*6+peak*.9+conflict*1.2+releaseDensity*1.4+std::max(0,releasePeak-1)*.8;
        s["skill_radar_components.HOLD.hold_lane_coverage"]=coverage;
    }
    s["skill_radar_components.HOLD.active_ln_lane_avg"]=avg;s["skill_radar_components.HOLD.active_ln_lane_peak"]=peak;s["skill_radar_components.HOLD.hold_tap_conflict_density"]=conflict;s["skill_radar_components.HOLD.release_density"]=releaseDensity;s["skill_radar_components.HOLD.release_chord_peak"]=releasePeak;
    raw[5]=holdRaw+p("ldb",90)*.22+p("ldbd",90)*.12+penalty({"lfds","lfda","lrds","lrda"})*7;
    std::array<std::string,6> scales{"stream_scale","speed_scale","jack_scale","chord_scale","tech_scale","hold_scale"};std::array<double,6> defaults{16,18,70,14,45,12};
    for(int k=0;k<6;k++){s["skill_radar_raw."+axes[k]]=roundn(raw[k]);double v=std::max(0.,raw[k]),scale=std::max(.001,c.get("skill_radar."+scales[k],defaults[k]));s["skill_radar."+axes[k]]=roundn(std::clamp(100*v/(v+scale),0.,100.),2);}
}
} // namespace revive
