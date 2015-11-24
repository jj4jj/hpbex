#include "mm.hpb.h"
#include <iostream>

int main(){

    using namespace mm;

    mm::Hero_ST hero;
    hero.construct();
    hero.id = 20;
    hero.append_id_list(2);
    hero.append_id_list(12);
    hero.append_id_list(255);

    std::cout << hero.find_idx_id_list(20) << std::endl;
    std::cout << hero.find_idx_id_list(255) << std::endl;

    mm::Hero   hero_pb;
    hero.convto(hero_pb);

    std::cout << hero_pb.ShortDebugString() << std::endl;
    return 0;
}
