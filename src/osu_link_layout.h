#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace osu_link::layout {
// Compatibility facts for 32-bit osu!stable, not imported reader code.
// Provenance and the deliberately unsupported clients are documented in
// docs/osu-link.md. Validation failure must hide the previous result.
inline constexpr std::array<unsigned char, 6> Anchor{0xf8, 0x01, 0x74, 0x04, 0x83, 0x65};
struct Identity {
    std::wstring folder, file, md5;
    bool operator==(const Identity& b) const { return folder==b.folder && file==b.file && md5==b.md5; }
    bool operator!=(const Identity& b) const { return !(*this==b); }
};
template<class Read> bool String(Read& read, std::uint32_t address, std::wstring& value, unsigned maximum) {
    std::uint32_t length=0;
    if (address<0x10000 || address>0xffffe000 || !read(address+4,&length,4) || !length || length>maximum) return false;
    value.resize(length);
    return read(address+8,value.data(),length*sizeof(wchar_t)) &&
        std::none_of(value.begin(),value.end(),[](wchar_t c){return c<32 || c==127;});
}
template<class Read> bool Decode(Read& read, std::uint32_t slot, Identity& result) {
    std::uint32_t object=0, again=0, folder=0, file=0, hash=0;
    if (slot<0x10000 || slot>0xffffff00 || !read(slot,&object,4) || object<0x10000 || object>0xfffff000) return false;
    if (!read(object+0x78,&folder,4) || !read(object+0x90,&file,4) || !read(object+0x6c,&hash,4)) return false;
    Identity next;
    if (!String(read,folder,next.folder,255) || !String(read,file,next.file,255) || !String(read,hash,next.md5,32) || next.md5.size()!=32) return false;
    if (!std::all_of(next.md5.begin(),next.md5.end(),[](wchar_t c){return (c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'f')||(c>=L'A'&&c<=L'F');})) return false;
    if (!read(slot,&again,4) || again!=object) return false; // selection changed during the snapshot
    for(auto& c:next.md5) if(c>=L'A'&&c<=L'F') c+=L'a'-L'A';
    result=std::move(next);
    return true;
}
}
