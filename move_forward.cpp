#include <iostream>
#include <vector>
#include <utility>

struct Status {
    int kind;
    int hp, id;
    int x, y;

    Status(){}
    Status(int akind, int ahp, int aid, int aowner, int ax, int ay)
        : kind(akind), hp(ahp), id(aid), x(ax), y(ay)
    {}
};

bool update()
{
    std::vector<Status> selfStatusList, enemyStatusList;
    int size = 0;   std::cin >> size;
    for(int i = 0;i < size;i++){
        Status st;
        std::cin >> st.id >> st.y >> st.x >> st.hp >> st.kind;
        selfStatusList.push_back(std::move(st));
    }
    std::cin >> size;
    for(int i = 0;i < size;i++){
        Status st;
        std::cin >> st.id >> st.y >> st.x >> st.hp >> st.kind;
        enemyStatusList.push_back(std::move(st));
    }

    std::cout << selfStatusList.size() << std::endl;
    for(auto&& st : selfStatusList){
        std::cout << st.id << " " << "U" << std::endl;
    }
}

int main()
{
    std::cout
        << "0 0 0  0 0 0  0 0 0  0 0 0  0 0 0  0 0 0  0 0 0" << std::endl
        << "3 3 3  0 0 0  0 0 0  4 4 4  0 0 0  0 0 0  3 3 3" << std::endl;
    for(int i = 0;i < 5;i++){
        update();
    }
}

