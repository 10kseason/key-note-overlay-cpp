"""Small deterministic calibration; stdlib only, no network or chart redistribution."""
import argparse
import bisect
import hashlib
import json
import math
from pathlib import Path
import statistics


def fit(rows, feature, bins):
    pairs = sorted((feature(r), r['level']) for r in rows)
    lo, hi = pairs[0][0], pairs[-1][0]
    buckets = [[] for _ in range(bins)]
    for x, y in pairs:
        buckets[min(bins-1, int((x-lo)/max(1e-9,hi-lo)*bins))].append((x,y))
    blocks = []
    for bucket in buckets:
        if not bucket:
            continue
        n = len(bucket)
        blocks.append([sum(x for x,_ in bucket), statistics.median(y for _,y in bucket)*n, n])
        while len(blocks) > 1 and blocks[-2][1]/blocks[-2][2] > blocks[-1][1]/blocks[-1][2]:
            b, a = blocks.pop(), blocks.pop()
            blocks.append([a[i]+b[i] for i in range(3)])
    curve = [(a/n,b/n) for a,b,n in blocks]
    if curve[0][0] > lo: curve.insert(0,(lo,curve[0][1]))
    if curve[-1][0] < hi: curve.append((hi,curve[-1][1]))
    return curve


def predict(curve, x):
    i = bisect.bisect_left([p[0] for p in curve], x)
    if i == 0: return curve[0][1]
    if i == len(curve): return curve[-1][1]
    a,b = curve[i-1],curve[i]
    return a[1]+(b[1]-a[1])*(x-a[0])/max(1e-9,b[0]-a[0])


def mae(rows, feature, curve):
    return statistics.mean(abs(predict(curve,feature(r))-r['level']) for r in rows)


def select(train, baseline=False):
    choices=[]
    for weight in ([0] if baseline else [0,1,2,4]):
        feature = (lambda r:r['v1']) if baseline else (lambda r,w=weight:r['stars']+w*math.log1p(r['nps']))
        for bins in [8,12,20,32]:
            losses=[]
            for fold in range(4):
                a=[r for r in train if r['fold']!=fold];b=[r for r in train if r['fold']==fold]
                if a and b: losses.append(mae(b,feature,fit(a,feature,bins)))
            choices.append((statistics.mean(losses),weight,bins))
    return min(choices)


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('features',type=Path)
    ap.add_argument('--header',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True)
    args=ap.parse_args()
    rows=[]
    for line in args.features.read_text().splitlines():
        ident,keys,level,group,stars,v1,nps=line.split('\t')
        rows.append(dict(id=ident,keys=int(keys),level=float(level),group=group,stars=float(stars),v1=float(v1),nps=float(nps),
                         test=int(group,16)%5==0,fold=(int(group,16)//5)%4))
    header=['#pragma once','#include <array>','// Generated from local labelled charts; see docs/calibration-report.json.',
            'namespace pattern {','struct CalibrationPoint { double strain; double level; };']
    report={'version':2,'split':'song-directory group hash: 20% untouched holdout, 4-fold train-only model selection',
            'target':'table label; features use mean of strongest three overlapping 8-second windows per chart',
            'evaluation_scope':'offline chart-level proxy, not measured live keyboard accuracy or a per-window confidence interval',
            'features_sha256':hashlib.sha256(args.features.read_bytes()).hexdigest(),
            'runtime':{'window_ms':8000,'warmup_context_ms':4000,'section_ms':400,'chord_anchor_ms':15,'hold_threshold_ms':90},
            'source_tables':{'5':'https://asumatoki.kr/table/aery/header.json','8':'http://walkure.net/hakkyou/for_glassist/bms/?lamp=normal'},
            'profiles':{}}
    for keys,name in [(5,'Aery'),(8,'Insane')]:
        subset=[r for r in rows if r['keys']==keys]
        train=[r for r in subset if not r['test']];test=[r for r in subset if r['test']]
        if len(train)<50 or len(test)<15: raise RuntimeError('Insufficient grouped calibration data')
        cv,weight,bins=select(train)
        baseline_cv,_,baseline_bins=select(train,True)
        feature=lambda r:r['stars']+weight*math.log1p(r['nps'])
        curve=fit(train,feature,bins)
        baseline=fit(train,lambda r:r['v1'],baseline_bins)
        residuals=sorted(abs(predict(curve,feature(r))-r['level']) for r in test)
        error80=residuals[min(len(residuals)-1,math.ceil(len(residuals)*0.8)-1)]
        header.append(f'inline constexpr std::array<CalibrationPoint, {len(curve)}> {name}Calibration = {{{{')
        header += [f'    {{{x:.9f}, {y:.9f}}},' for x,y in curve]
        header += ['}};',f'inline constexpr double {name}Error80 = {error80:.9f};',f'inline constexpr double {name}DensityWeight = {weight:.1f};']
        result=dict(charts=len(subset),train=len(train),test=len(test),train_groups=len({r['group'] for r in train}),
                    test_groups=len({r['group'] for r in test}),overlapping_groups=len({r['group'] for r in train}&{r['group'] for r in test}),
                    label_range=[min(r['level'] for r in subset),max(r['level'] for r in subset)],
                    train_cv_mae=cv,baseline_train_cv_mae=baseline_cv,bins=bins,density_weight=weight,
                    holdout_mae=mae(test,feature,curve),baseline_calibrated_v1_holdout_mae=mae(test,lambda r:r['v1'],baseline),
                    holdout_median_absolute_error=statistics.median(residuals),holdout_error80=error80,
                    runtime_feature_range=[curve[0][0],curve[-1][0]],
                    below_training_range=sum(feature(r)<curve[0][0] for r in test),above_training_range=sum(feature(r)>curve[-1][0] for r in test))
        report['profiles'][str(keys)]=result
        print(name,json.dumps(result))
    header.append('}')
    args.header.write_text('\n'.join(header)+'\n',encoding='utf-8')
    args.report.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')


if __name__=='__main__':main()
