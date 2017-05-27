#include "hoolib.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

// for animation gif
//#include <boost/format.hpp>
//#include "SvgDrawer.hpp"
//#include <sstream>

// for PopenPlayer
#include <boost/process.hpp>
namespace bp = boost::process;

enum {
    FIELD_WIDTH = 7, FIELD_HEIGHT = 7,
    SELF_ZONE_HEIGHT = 2,
};

enum class DIRECTION { LEFT, UP, RIGHT, DOWN };

class Pos
{
private:
    int x_, y_;

public:
    Pos(int index)
        : x_(index % FIELD_WIDTH), y_(index / FIELD_WIDTH)
    {}

    Pos(int x, int y)
        : x_(x), y_(y)
    {}

    bool isValid() const { return (0 <= x_ && x_ < FIELD_WIDTH && 0 <= y_ && y_ < FIELD_HEIGHT); }

    int getX() const { return x_; }
    int getY() const { return y_; }
    int getIndex() const { return x_ + y_ * FIELD_WIDTH; }

    Pos getMoved(DIRECTION dir) const
    {
        Pos ret(*this);
        switch(dir)
        {
        case DIRECTION::LEFT:   ret.x_--;  break;
        case DIRECTION::UP:     ret.y_--;  break;
        case DIRECTION::RIGHT:  ret.x_++;  break;
        case DIRECTION::DOWN:   ret.y_++;  break;
        }
        return ret;
    }

    bool operator==(const Pos& rhs) const
    {
        return x_ == rhs.x_ && y_ == rhs.y_;
    }
};

class Soldier;
using SoldierPtr = std::shared_ptr<Soldier>;
using SoldierPtrList = std::vector<SoldierPtr>;
class Soldier
{
public:
    enum KIND { KNIGHT = 0, FIGHTER, ASSASSIN };
    struct Status {
        KIND kind;
        int hp, id, owner;
        Pos pos;
        Status(KIND akind, int ahp, int aid, int aowner, const Pos& apos)
            : kind(akind), hp(ahp), id(aid), owner(aowner), pos(apos)
        {}
    };

private:
    Status status_;

private:
    Soldier(const Status& status)
        : status_(status)
    {}

    // thanks to http://d.hatena.ne.jp/gintenlabo/20131211/1386771626
    static SoldierPtr createSoldierPtr(const Status& status)
    {
        struct Impl : Soldier { Impl(const Status& status) : Soldier(status){} };
        auto p = std::make_shared<Impl>(status);
        return std::move(p);
    }

public:
    virtual ~Soldier(){}

    static SoldierPtr createKnight(int id, int owner, const Pos& pos) { return createSoldierPtr(Status(KIND::KNIGHT, 200, id, owner, pos)); }
    static SoldierPtr createFighter(int id, int owner, const Pos& pos) { return createSoldierPtr(Status(KIND::FIGHTER, 200, id, owner, pos)); }
    static SoldierPtr createAssassin(int id, int owner, const Pos& pos) { return createSoldierPtr(Status(KIND::ASSASSIN, 200, id, owner, pos)); }

    bool isAlive() const { return status_.hp > 0; }
    bool isDead() const { return !isAlive(); }

    const Status& getStatus() const { return status_; }

    void moveTo(const Pos& pos) { status_.pos = pos; }
    void setHP(int hp) { status_.hp = hp; }
};

class SoldierPtrColony
{
private:
    SoldierPtrList src_;

public:
    SoldierPtrColony(SoldierPtrList src)
        : src_(src)
    {}

    const SoldierPtrList& get() const { return src_; }

    template<class Prod>
    SoldierPtrColony filter(Prod prod) const
    {
        SoldierPtrList src;
        std::copy_if(HOOLIB_RANGE(src_), std::back_inserter(src), prod);
        return SoldierPtrColony(src);
    }

    SoldierPtr getFromId(int id)
    {
        return filter([id](const SoldierPtr& solptr) { return solptr->getStatus().id == id; }).get().front();
    }

    SoldierPtrColony byKind(Soldier::KIND kind) const
    {
        return filter([kind](const SoldierPtr& solptr) { return solptr->getStatus().kind == kind; });
    }

    SoldierPtrColony byOwner(int owner) const
    {
        return filter([owner](const SoldierPtr& solptr) { return solptr->getStatus().owner == owner; });
    }

    SoldierPtrColony byAlive() const
    {
        return filter([](const SoldierPtr& solptr) { return solptr->isAlive(); });
    }

    SoldierPtrColony byPos(const Pos& pos) const
    {
        return filter([pos](const SoldierPtr& solptr) { return solptr->getStatus().pos == pos; });
    }
};

int getDamage(Soldier::KIND attacker, Soldier::KIND defender)
{
    static int table[3][3] = {
      // KNI, FIG, ASA
        {150, 100, 200}, // KNIGHT
        {200, 150, 100}, // FIGHTER
        {100, 200, 150}, // ASSASSIN
    };

    return table[static_cast<int>(attacker)][static_cast<int>(defender)];
}

struct MoveInstruction
{
    int id;
    DIRECTION dir;
    MoveInstruction(int aid, DIRECTION adir)
        : id(aid), dir(adir)
    {}
};
using MoveInstructionList = std::vector<MoveInstruction>;

class Stage
{
public:
    struct BiasedStatus
    {
        int selfOwnerId;
        std::vector<Soldier::Status> self, enemy;
    };

private:
    SoldierPtrColony soldiers_;

public:
    Stage(const SoldierPtrList& src)
        : soldiers_(src)
    {
    }

    ~Stage(){}

    BiasedStatus getBiasedStatus(int selfOwnerId)
    {
        auto self = soldiers_.byOwner(selfOwnerId).byAlive(), enemy = soldiers_.byOwner(selfOwnerId == 0 ? 1 : 0).byAlive();
        BiasedStatus ret;
        for(auto&& solptr : self.get())
            ret.self.push_back(solptr->getStatus());
        for(auto&& solptr : enemy.get())
            ret.enemy.push_back(solptr->getStatus());
        return ret;
    }

    void dump(std::ostream& os = std::cout) const
    {
        for(int y = 0;y < FIELD_HEIGHT;y++){
            for(int kind = 0;kind < 3;kind++){
                for(int x = 0;x < FIELD_WIDTH;x++){
                    auto solptrs = soldiers_.byPos(Pos(x, y)).byAlive();
                    os << std::setw(2) << std::setfill('0')
                        << solptrs.byOwner(0).byKind(static_cast<Soldier::KIND>(kind)).get().size()
                        << " ";
                    os << std::setw(2) << std::setfill('0')
                        << solptrs.byOwner(1).byKind(static_cast<Soldier::KIND>(kind)).get().size()
                        << "  ";
                }
                os << std::endl;
            }
            os << std::endl;
        }

        auto& src = soldiers_.get();
        std::cout << "id kind owner hp x y" << std::endl;
        for(int i = 0;i < src.size();i++){
            auto st = src[i]->getStatus();
            std::cout << st.id << " " << static_cast<int>(st.kind) << " " << st.owner << " " << st.hp << " " << st.pos.getX() << " " << st.pos.getY() << std::endl;
        }
    }

    void move(const MoveInstructionList& moiList)
    {
        for(auto&& moi : moiList){
            auto& soldier = *soldiers_.getFromId(moi.id);
            auto pos = soldier.getStatus().pos.getMoved(moi.dir);
            HOOLIB_THROW_UNLESS(pos.isValid(), "pos is invalid.");
            soldier.moveTo(pos);
        }
    }

    void update()
    {
        static HooLib::multi_array<int, 13, 2> dxdyTable = {
                          +0,-2,
                   -1,-1, +0,-1, +1,-1,
            -2,+0, -1,+0, +0,+0, +1,+0, +2,+0,
                   -1,+1, +0,+1, +1,+1,
                          +0,+2
        };

        struct Battle
        {
            SoldierPtr attacker;
            SoldierPtrList targetAll;
            int k;
        };

        std::vector<Battle> battles;
        for(int owner = 0;owner < 2;owner++){
            auto attackers = soldiers_.byOwner(owner).byAlive().get();
            for(auto&& attacker : attackers){
                Battle battle;
                battle.attacker = attacker;
                battle.k = 0;
                Pos atkPos = attacker->getStatus().pos;

                for(auto&& dxdy : dxdyTable){
                    auto targetPos = Pos(atkPos.getX() + dxdy[0], atkPos.getY() + dxdy[1]);
                    if(!targetPos.isValid()) continue;
                    auto targetSolPtrs = soldiers_.byOwner(owner == 0 ? 1 : 0).byPos(targetPos).byAlive();
                    if(targetSolPtrs.get().empty())   continue;
                    battle.k += HooLib::min(10, targetSolPtrs.get().size());
                    battle.targetAll.insert(battle.targetAll.end(), HOOLIB_RANGE(targetSolPtrs.get()));
                }
                battles.push_back(battle);
            }
        }
        for(auto&& battle : battles){
            for(auto& target: battle.targetAll){
                auto hp = target->getStatus().hp;
                target->setHP(hp - getDamage(battle.attacker->getStatus().kind, target->getStatus().kind) / battle.k);
            }
        }
    }
};

class Player
{
public:
    Player(){}
    virtual ~Player(){}

    virtual HooLib::multi_array<int, FIELD_WIDTH * SELF_ZONE_HEIGHT, 3> buildInitialArrangement() = 0;
    virtual std::vector<MoveInstruction> think(const std::vector<Soldier::Status>& self, const std::vector<Soldier::Status>& enemy) = 0;
};

class PopenPlayer : public Player
{
private:
    bp::opstream opipe_;
    bp::ipstream ipipe_;
    std::shared_ptr<bp::child> proc_;
    HooLib::multi_array<int, FIELD_WIDTH * SELF_ZONE_HEIGHT, 3> initialArrangement_;

public:
    PopenPlayer(const std::string& command)
    {
        proc_ = std::make_shared<bp::child>(command, bp::std_in < opipe_, bp::std_out > ipipe_);

        // read initial arrangement
        for(int i = 0;i < SELF_ZONE_HEIGHT;i++){
            std::string input;
            HOOLIB_THROW_UNLESS(ipipe_ && std::getline(ipipe_, input) && !input.empty(), "input pipe doesn't work correctly.(3)");
            auto tokens = HooLib::splitStrByChars(input, " \t");
            HOOLIB_THROW_UNLESS(tokens.size() == FIELD_WIDTH * 3, "invalid field width.");
            for(int j = 0;j < FIELD_WIDTH;j++){
                for(int k = 0;k < 3;k++){
                    int num = HooLib::str2int(tokens[j * 3 + k]);
                    initialArrangement_[i * FIELD_WIDTH + j][k] = num;
                }
            }
        }
    }


    HooLib::multi_array<int, FIELD_WIDTH * SELF_ZONE_HEIGHT, 3> buildInitialArrangement() override
    {
        return initialArrangement_;
    }

    std::vector<MoveInstruction> think(const std::vector<Soldier::Status>& self, const std::vector<Soldier::Status>& enemy) override
    {
        // input
        opipe_ << self.size() << std::endl;
        for(auto&& st: self)
            opipe_
                << st.id << " "
                << st.pos.getY() << " "
                << st.pos.getX() << " "
                << st.hp << " "
                << static_cast<int>(st.kind) << std::endl;
        opipe_ << enemy.size() << std::endl;
        for(auto&& st: enemy)
            opipe_
                << st.id << " "
                << st.pos.getY() << " "
                << st.pos.getX() << " "
                << st.hp << " "
                << static_cast<int>(st.kind) << std::endl;

        // output
        std::vector<MoveInstruction> moiList;
        std::string input;
        HOOLIB_THROW_UNLESS(ipipe_ && std::getline(ipipe_, input) && !input.empty(), "input pipe doesn't work correctly.(1)");
        int n = HooLib::str2int(input);
        for(int i = 0;i < n;i++){
            HOOLIB_THROW_UNLESS(ipipe_ && std::getline(ipipe_, input) && !input.empty(), "input pipe doesn't work correctly.(2)");
            auto tokens = HooLib::splitStrByChars(input, " \t");
            HOOLIB_THROW_UNLESS(tokens.size() == 2, "tokens' size is invalid.");
            HOOLIB_THROW_UNLESS(!tokens[1].empty(), "tokens are nothing.");
            int id = HooLib::str2int(tokens[0]);
            DIRECTION dir;
            switch(tokens[1][0]){
            case 'L': case 'l':
                dir = DIRECTION::LEFT;
                break;
            case 'U': case 'u':
                dir = DIRECTION::UP;
                break;
            case 'R': case 'r':
                dir = DIRECTION::RIGHT;
                break;
            case 'D': case 'd':
                dir = DIRECTION::DOWN;
                break;
            }
            moiList.emplace_back(id, dir);
        }

        return std::move(moiList);
    }
};

class RandomPlayer : public Player
{
public:
    RandomPlayer(){}
    ~RandomPlayer(){}

    HooLib::multi_array<int, FIELD_WIDTH * SELF_ZONE_HEIGHT, 3> buildInitialArrangement() override
    {
        HooLib::multi_array<int, 14, 3> ret = {
            0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
            0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0
        };

        for(int k = 0;k < 3;k++)
            for(int i = 0;i < 10;i++)
                ret[HooLib::randomInt(0, 14)][k]++;

        return std::move(ret);
    }

    std::vector<MoveInstruction> think(const std::vector<Soldier::Status>& self, const std::vector<Soldier::Status>& enemy) override
    {
        std::vector<MoveInstruction> moiList;
        for(auto&& solst : self){
            //static auto randGen = std::mt19937(std::random_device()());
            static auto randGen = std::mt19937(::time(NULL));   // for MinGW
            std::array<DIRECTION, 4> dirTable = {
                DIRECTION::LEFT, DIRECTION::UP, DIRECTION::RIGHT, DIRECTION::DOWN
            };
            std::shuffle(HOOLIB_RANGE(dirTable), randGen);
            for(auto&& dir : dirTable){
                auto pos = solst.pos.getMoved(dir);
                if(!pos.isValid())  continue;
                moiList.emplace_back(solst.id, dir);
                break;
            }
        }
        return moiList;
    }
};

/*
void drawStageStatus(const Stage::BiasedStatus& status, const std::string& filename)
{
    static std::array<HooLib::RGB, 3> colorTable = {
        HooLib::RGB::red(), HooLib::RGB::green(), HooLib::RGB::blue()
    };
    HooLib::multi_array<int, FIELD_WIDTH, FIELD_HEIGHT, 2, 3> map = {};
    for(auto&& soldier : status.self)
        map[soldier.pos.getX()][soldier.pos.getY()][0][static_cast<int>(soldier.kind)]++;
    for(auto&& soldier : status.enemy)
        map[soldier.pos.getX()][soldier.pos.getY()][1][static_cast<int>(soldier.kind)]++;

    std::vector<std::pair<HooLib::Rect, HooLib::RGB>> rects;
    for(int y = 0;y < FIELD_HEIGHT;y++)
        for(int x = 0;x < FIELD_WIDTH;x++)
            for(int o = 0;o < 2;o++)
                for(int k = 0;k < 3;k++)
                    if(map[x][y][o][k] != 0)
                        rects.push_back(std::make_pair(
                            HooLib::XYWH(
                                x * 100 + 50 * o,
                                y * 100 + k * 33,
                                map[x][y][o][k] * 5,
                                33),
                            colorTable[k]));

    mi::SvgDrawer drawer(700, 700, filename);
    drawer.setViewBox(0, 0, 700, 700);
    for(int y = 0;y <= FIELD_HEIGHT;y++)
        drawer.drawLine(0, y * 100, 100 * FIELD_WIDTH, y * 100);
    for(int x = 0;x <= FIELD_WIDTH;x++)
        drawer.drawLine(x * 100, 0, x * 100, 100 * FIELD_HEIGHT);
    for(auto&& src : rects){
        auto& rect = src.first;
        auto& color = src.second;
        auto colorStr = (boost::format("#%02x%02x%02x") % color.r() % color.g() % color.b()).str();
        drawer.setFillColor(colorStr);
        drawer.setStrokeColor(colorStr);
        drawer.drawRect(rect.x(), rect.y(), rect.width() - 1, rect.height() - 1);
    }
}
*/

int main()
{
    /*
    HooLib::multi_array<int, 2, 49, 3> src = {
    //int src[2][49][3] = {  // KNI, FIG, ASA
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,

        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0
    };

    for(int k = 0;k < 3;k++){
        for(int i = 0;i < 10;i++){
            src[0][35 + HooLib::randomInt(0, 14)][k]++;
            src[1][HooLib::randomInt(0, 14)][k]++;
        }
    }

    SoldierPtrList solList;
    int id = 0;
    for(int a = 0;a < 2;a++){
        for(int b = 0;b < 49;b++){
            for(int c = 0;c < 3;c++){
                for(int i = 0;i < src[a][b][c];i++){
                    switch(c){
                    case 0: solList.push_back(Soldier::createKnight(id++, a, Pos(b)));  break;
                    case 1: solList.push_back(Soldier::createFighter(id++, a, Pos(b)));  break;
                    case 2: solList.push_back(Soldier::createAssassin(id++, a, Pos(b)));  break;
                    }
                }
            }
        }
    }
    */

    std::shared_ptr<Player> players[2];
    //players[0] = std::make_shared<RandomPlayer>();
    //players[1] = std::make_shared<RandomPlayer>();
    players[0] = std::make_shared<PopenPlayer>("./move_forward");
    players[1] = std::make_shared<PopenPlayer>("./move_forward");

    SoldierPtrList solList;
    for(int p = 0;p < 2;p++){
        auto src = players[p]->buildInitialArrangement();
        for(int a = 0;a < FIELD_WIDTH * SELF_ZONE_HEIGHT;a++){
            Pos pos(p == 0 ? ((FIELD_HEIGHT - SELF_ZONE_HEIGHT) * FIELD_WIDTH + a) : (FIELD_WIDTH * SELF_ZONE_HEIGHT - (a + 1)));
            for(int b = 0;b < 3;b++){
                for(int i = 0;i < src[a][b];i++){
                    switch(b){
                    case 0:
                        solList.push_back(Soldier::createKnight(solList.size(), p, pos));
                        break;
                    case 1:
                        solList.push_back(Soldier::createFighter(solList.size(), p, pos));
                        break;
                    case 2:
                        solList.push_back(Soldier::createAssassin(solList.size(), p, pos));
                        break;
                    }
                }
            }
        }
    }

    Stage stage(solList);

    stage.dump();
    //drawStageStatus(stage.getBiasedStatus(0), "pic/test000.svg");
    for(int turn = 0;turn < 100;turn++){
        std::vector<MoveInstruction> moiList;
        for(int owner = 0;owner < 2;owner++){
            auto status = stage.getBiasedStatus(owner);
            auto tmp = players[owner]->think(status.self, status.enemy);
            if(owner == 1){ // reverse direction
                for(auto& moi : tmp){
                    switch(moi.dir){
                    case DIRECTION::LEFT:  moi.dir = DIRECTION::RIGHT; break;
                    case DIRECTION::UP:    moi.dir = DIRECTION::DOWN;  break;
                    case DIRECTION::RIGHT: moi.dir = DIRECTION::LEFT;  break;
                    case DIRECTION::DOWN:  moi.dir = DIRECTION::UP;    break;
                    }
                }
            }
            moiList.insert(moiList.end(), HOOLIB_RANGE(tmp));
        }
        stage.move(moiList);
        stage.update();
        std::cout << turn << "===" << std::endl;
        stage.dump();
        //drawStageStatus(stage.getBiasedStatus(0), (boost::format("pic/test%03d.svg") % (turn + 1)).str());
    }
}
