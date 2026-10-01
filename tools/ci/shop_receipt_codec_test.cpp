#include "custom/shop_receipt_codec.hpp"
#include <cassert>
#include <iostream>
#include <random>
int main() {
    using namespace pn_shop_receipt_codec;
    std::mt19937 random(20261001);
    for (unsigned trial = 0; trial < 250; ++trial) {
        std::vector<unsigned char> raw(59954);
        for (auto& byte : raw) byte = trial % 2 ? random() : (random()%100 < 2 ? random() : 0);
        auto packed = encode(raw.data(), raw.size());
        assert(matches(raw.data(), raw.size(), raw.data(), raw.size()));
        assert(matches(packed.data(), packed.size(), raw.data(), raw.size()));
        auto changed = raw;changed[random()%changed.size()] ^= 1;
        assert(!matches(packed.data(), packed.size(), changed.data(), changed.size()));
        assert(packed.size() <= raw.size());
    }
    std::vector<unsigned char> raw(59954);raw[0]=0x98;raw[1]=0x30;
    auto packed=encode(raw.data(),raw.size());assert(packed.size()<1000);
    for(size_t n=0;n<packed.size();++n)
        assert(!matches(packed.data(),n,raw.data(),raw.size()));
    for(size_t n=0;n<packed.size();++n)for(unsigned bit=0;bit<8;++bit){
        auto bad=packed;bad[n]^=1<<bit;
        // Deflate contains unused padding bits; corruption there may leave the
        // exact decoded request unchanged. Never accept a different request.
        if(matches(bad.data(),bad.size(),raw.data(),raw.size())) {
            auto changed=raw;changed[20]=1;
            assert(!matches(bad.data(),bad.size(),changed.data(),changed.size()));
        }
    }
    auto bad=packed;bad.push_back(0);assert(!matches(bad.data(),bad.size(),raw.data(),raw.size()));
    bad=packed;bad[4]=2;assert(!matches(bad.data(),bad.size(),raw.data(),raw.size()));
    bad=packed;bad[8]^=1;assert(!matches(bad.data(),bad.size(),raw.data(),raw.size()));
    std::vector<unsigned char> bomb(limit);auto large=encode(bomb.data(),bomb.size());
    assert(!matches(large.data(),large.size(),raw.data(),raw.size()));
    for(unsigned i=0;i<4;++i)large[sizeof(magic)+i]=static_cast<unsigned char>(raw.size()>>(8*i));
    assert(!matches(large.data(),large.size(),raw.data(),raw.size())); // forged short header, oversized inflation
    std::memcpy(raw.data(),magic,sizeof(magic));
    assert(matches(raw.data(),raw.size(),raw.data(),raw.size()));
    assert(!matches(nullptr,0,raw.data(),raw.size()));
    std::cout<<"PASS receipt codec: raw compatibility, exact identity, bounded decode, truncation, version, trailing bytes and corruption\n";
}
