#include <algorithm>
#include <atlbase.h>
#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <random>

namespace {
// 単位時間あたりの移動量設定
const constexpr int kDivUnit = 45;
const constexpr int kTimer = (1000 / kDivUnit);
const constexpr int kSleep = 16;

// TODO:定型文
static const constexpr char kWindowTitle[] = "FEもどき";
static const constexpr char kErrorMessage[] = "エラー";


/*ウィンドウの形など*/
const constexpr int kWindowWidth = 800;
const constexpr int kWindowHeight = 600;
const constexpr int kWindowTop = 50;
const constexpr int kWindowLeft = 50;

const constexpr int kUndefined = -1;

// マップサイズ
const constexpr int kMapWidth = 30;
const constexpr int kMapHeight = 13;

// ユニット関連の定数
const constexpr int kMinHp = 0;
}

// 乱数生成器
std::mt19937 randomizer_;

struct MapPosition {
    int x;
    int y;
};

// x,y方向の移動量
struct MoveVector {
    int x;
    int y;
};

// ゲームのフェーズ
enum Phase {
    kStartAlly,
    kSelectUnit,
    kSetMovePosition,
    kSelectAttackUnit,
    kStartEnemy,

    kPhaseMax,
};
Phase phase_ = Phase::kSelectUnit;

enum Turn {
    kAllyTurn,
    kEnemyTurn,

    kTurnMax,
};
Turn turn_ = Turn::kAllyTurn;

// 方角
enum Direction {
    kNorth,
    kWest,
    kSouth,
    kEast,

    kDirectionMax,
};

// 各方角の移動量
const constexpr MoveVector directions[Direction::kDirectionMax] = {
    {  0, -1 }, // kNorth
    { -1,  0 }, // kWest
    {  0,  1 }, // kSouth
    {  1,  0 }, // kEast
};

enum Cell {
    kSea,
    kPlane,
    kForest,
    kMountain,
    kRock,
    kTown,
    kFort,
    kCastle,

    kCellMax
};

struct CellDescription {
    std::string name;
    std::string aa;
    int defence;
    bool heal;
};

std::vector<CellDescription> cell_list_ = {
    { "海",      "～",   30, false},
    { "草原",    "　",     5, false},
    { "林",      "林",   15, false},
    { "山",      "へ",    25, false},
    { "高山",    "△",     0, false},
    { "町",      "町",    0, false},
    { "砦",      "砦",    20, false},
    { "城",      "城",    30, false},
};

char map_data_[] = "\
000000000000000000000000000000\
0000kkk00000000000000000000000\
0001h12400000000000555e1252100\
001111144000000611111111f15110\
011171k44400001111121111111110\
01k11124444441k11111000bag1110\
0111kk124442111211100001611110\
011511i133211111100001cd111100\
01111113111k1k1220001111111000\
000111111112110000001121110000\
00111k116111112200021j11210000\
000015500001100000000000220000\
000000000000000000000000000000";
int cells[kMapHeight][kMapWidth];

// 移動先塗りつぶし判定用
bool fill[kMapHeight][kMapWidth];

// 職業
enum Job {
    kLoad,
    kParadin,
    kSKnight,
    kAKnight,
    kArcher,
    kPKnight,
    kPirate,
    kHunter,
    kThief,

    kJobMax,
};
struct JobDescription {
    std::string name;
    std::string aa;
    int consts[Cell::kCellMax];
};
std::vector<JobDescription> job_list_ = {
    { "ロード",     "君", { -1, 1, 2,  4, -1, 1, 2, 2 } },
    { "パラディン", "騎", { -1, 1, 3,  6, -1, 1, 2, 2 } },
    { "Sナイト",    "騎", { -1, 1, 3, -1, -1, 1, 2, 2 } },
    { "Aナイト",    "重", { -1, 1, 2, -1, -1, 1, 2, 2 } },
    { "アーチャー", "射", { -1, 1, 3, -1, -1, 1, 2, 2 } },
    { "Pナイト",    "天", {  1, 1, 1,  1,  1, 1, 1, 1 } },
    { "海賊",       "海", {  2, 1, 2,  4, -1, 1, 2, 2 } },
    { "ハンター",   "狩", { -1, 1, 2,  3, -1, 1, 2, 2 } },
    { "盗賊",       "盗", { -1, 1, 2,  4, -1, 1, 2, 2 } }
};

// 武器
enum Weapon {
    kIronSword,
    kRapier,
    kSpear,
    kSilverSpear,
    kHandSpear,
    kBow,
    kStealBow,
    kCrossBow,
    kAxe,
    kStealAxe,

    kWeaponMax,
};
struct WeaponDescription {
    std::string name;
    int power;
    int weight;
    int hit;
    int critical;
    int range_min;
    int range_max;
};
std::vector<WeaponDescription> Weapon_list_ = {
    { "鉄剣",    5, 2, 100,  0, 1, 1 },
    { "刺突剣",  5, 1, 100, 10, 1, 1 },
    { "槍",      8, 6,  80,  0, 1, 1 },
    { "銀槍",   12, 7,  80,  0, 1, 1 },
    { "手槍",    7, 6,  70,  0, 1, 2 },
    { "木弓",    4, 1,  90,  0, 2, 2 },
    { "鋼弓",    7, 3,  80,  0, 2, 2 },
    { "弩",      5, 2, 100, 20, 2, 2 },
    { "斧",      7, 7,  80,  0, 1, 1 },
    { "鋼斧",    9, 9,  70,  0, 1, 1 },
};

enum Team {
    kAlly,
    kEnemy,

    kTeamMax,
};

// ユニット
struct UnitDescription {
    std::string name;
    Job job;
    int max_hp;
    int strength;
    int skill;
    int weapon_level;
    int agility;
    int luck;
    int defence;
    int move;
    Team team;
    Weapon weapon;
    MapPosition position;
    int hp;

    // 行動済み判定
    bool done;
};
std::vector<UnitDescription> unit_list_ = {
    { "マルス",     Job::kLoad,    18, 5,  3,  5,  7, 7,  7,  7, Team::kAlly,  Weapon::kRapier },
    { "ジェイガン", Job::kParadin, 20, 7, 10, 10,  8, 1,  9, 10, Team::kAlly,  Weapon::kIronSword },
    { "カイン",     Job::kSKnight, 18, 7,  5,  5,  6, 3,  7,  9, Team::kAlly,  Weapon::kSpear },
    { "アベル",     Job::kSKnight, 18, 6,  7,  6,  7, 2,  7,  9, Team::kAlly,  Weapon::kIronSword },
    { "ドーガ",     Job::kAKnight, 16, 7,  3,  4,  3, 1, 11,  5, Team::kAlly,  Weapon::kIronSword },
    { "ゴードン",   Job::kArcher,  16, 5,  1,  5,  4, 4,  6,  5, Team::kAlly,  Weapon::kCrossBow },
    { "シーダ",     Job::kPKnight, 16, 3,  6,  7, 12, 9,  7,  8, Team::kAlly,  Weapon::kIronSword },
    { "ガザック",   Job::kPirate,  24, 7,  3,  7,  8, 0,  6,  6, Team::kEnemy, Weapon::kStealAxe },
    { "ガルダ兵",   Job::kHunter,  18, 6,  1,  5,  5, 0,  3,  6, Team::kEnemy, Weapon::kBow },
    { "ガルダ兵",   Job::kThief,   16, 3,  1,  2,  9, 0,  2,  7, Team::kEnemy, Weapon::kIronSword },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
};

/// <summary>
/// 指定座標のユニットインデックス取得
/// </summary>
/// <param name="search_position"></param>
/// <returns></returns>
int GetUnitIndex(MapPosition search_position) {
    auto unit = std::find_if(unit_list_.begin(), unit_list_.end(), [&](UnitDescription obj) {
        return (obj.position.x == search_position.x && obj.position.y == search_position.y);
    });

    if (unit == unit_list_.end()) {
        return kUndefined;
    }
    int result = std::distance(unit_list_.begin(), unit);
    return result;
}

/// <summary>
/// 指定チームの行動済み判定をリセットする。
/// </summary>
/// <param name="turn"></param>
void ResetDone(Team team) {
    for (auto& unit : unit_list_) {
        if (unit.team == team) {
            unit.done = false;
        }
    }
}

/// <summary>
/// 数値を0埋めした文字列に整形する
/// </summary>
/// <param name="digit"></param>
/// <param name="value"></param>
/// <returns></returns>
std::string FormatFillDigitWithZero(const int digit, const int value) {
    std::ostringstream wos;
    wos << std::setw(digit) << std::setfill(_T('0')) << value;
    return wos.str();
}

/// <summary>
/// ユニットが装備している武器情報表示
/// </summary>
/// <param name="weapon"></param>
/// <returns></returns>
std::string DisplayUnitWeaponParameter(Weapon weapon) {
    WeaponDescription weapon_data = Weapon_list_[weapon];
    std::string weapon_parameter;
    weapon_parameter += weapon_data.name;
    weapon_parameter += " (ダメージ:" + std::to_string(weapon_data.power);
    weapon_parameter += " 重さ:" + std::to_string(weapon_data.weight);
    weapon_parameter += " 命中率:" + std::to_string(weapon_data.hit);
    weapon_parameter += " 必殺率:" + std::to_string(weapon_data.critical);
    weapon_parameter += " 射程:" + std::to_string(weapon_data.range_min) + "～" + std::to_string(weapon_data.range_max);
    weapon_parameter += ")\n";

    return weapon_parameter;
}

/// <summary>
/// ユニット情報表示
/// </summary>
/// <param name="index"></param>
/// <returns></returns>
std::string DisplayUnitParameter(int index) {
    if (index == kUndefined) {
        return "";
    }

    std::string parameter;
    parameter += unit_list_[index].name;
    if (unit_list_[index].team == Team::kAlly) {
        parameter += "(";
        parameter += (unit_list_[index].done ? "行動済" : "未行動");
        parameter += ")";
    }
    parameter += "\n";
    parameter += job_list_[unit_list_[index].job].name + "\n";

    const int kDigit = 2;
    parameter += "HP　　　　：" + FormatFillDigitWithZero(kDigit, unit_list_[index].hp) + "/";
    parameter += FormatFillDigitWithZero(kDigit, unit_list_[index].max_hp) + "\n";
    parameter += "力　　　　：" + FormatFillDigitWithZero(kDigit, unit_list_[index].strength) + "\n";
    parameter += "技　　　　：" + FormatFillDigitWithZero(kDigit, unit_list_[index].skill) + "\n";
    parameter += "武器レベル：" + FormatFillDigitWithZero(kDigit, unit_list_[index].weapon_level) + "\n";
    parameter += "素早さ　　：" + FormatFillDigitWithZero(kDigit, unit_list_[index].agility) + "\n";
    parameter += "運　　　　：" + FormatFillDigitWithZero(kDigit, unit_list_[index].luck) + "\n";
    parameter += "防御力　　：" + FormatFillDigitWithZero(kDigit, unit_list_[index].defence) + "\n";
    parameter += "移動力　　：" + FormatFillDigitWithZero(kDigit, unit_list_[index].move) + "\n";

    parameter += DisplayUnitWeaponParameter(unit_list_[index].weapon);

    return parameter;
}

/// <summary>
/// 地形情報表示
/// </summary>
/// <param name="position"></param>
/// <returns></returns>
std::string DisplayCellParameter(MapPosition position) {
    std::string parameter;
    CellDescription cell = cell_list_[cells[position.y][position.x]];

    parameter += cell.name + "\n";

    const int kDigit = 2;
    parameter += "防御効果:" + FormatFillDigitWithZero(kDigit, cell.defence) + "%\n";

    parameter += "回復効果:";
    parameter += (cell.heal ? "あり" : "なし");
    parameter += "\n";

    return parameter;
}

/// <summary>
/// フェーズガイダンス表示
/// </summary>
/// <param name="phase"></param>
/// <returns></returns>
std::string DisplayPhaseGuidance(Phase phase) {
    std::string result = "";
    switch (phase_) {
    case kStartAlly:        result = "味方のターン";  break;
    case kSelectUnit:       result = "ユニットを選択してください。(e:ターン終了)\n\n";  break;
    case kSetMovePosition:  result = "移動先を設定してください。\n\n";  break;
    case kSelectAttackUnit: result = "攻撃対象を選んでください。(自身を選択で待機)\n\n";  break;
    case kStartEnemy:       result = "敵のターン";  break;
    default:
        break;
    }
    return result;
}

// カーソル
MapPosition cursor_position{0, 0};

// 選択中ユニット
int selected_unit = kUndefined;

/// <summary>
/// 移動可能か確認する。
/// </summary>
/// <param name="unit_index"></param>
/// <param name="search_position"></param>
/// <param name="remain_move"></param>
/// <returns></returns>
bool CanMove(int unit_index, MapPosition search_position, int remain_move) {
    if (search_position.x < 0 || kMapWidth <= search_position.x) {
        return false;
    }

    if (search_position.y < 0 || kMapHeight <= search_position.y) {
        return false;
    }

    int search_unit_index = GetUnitIndex(search_position);
    if (kUndefined < search_unit_index && unit_list_[unit_index].team != unit_list_[search_unit_index].team) {
        return false;
    }

    int move_cost = job_list_[unit_list_[unit_index].job].consts[cells[search_position.y][search_position.x]];

    // 移動不可の地形の場合、スキップする
    if (move_cost <= kUndefined) {
        return false;
    }

    if (remain_move < move_cost) {
        return false;
    }

    return true;
}

/// <summary>
/// 移動可能範囲の塗りつぶし判定更新
/// </summary>
/// <param name="unit_index"></param>
/// <param name="search_position"></param>
/// <param name="remain_move"></param>
void FillCanMoveCells(int unit_index, MapPosition search_position, int remain_move) {
    if (!CanMove(unit_index, search_position, remain_move)) {
        return;
    }

    fill[search_position.y][search_position.x] = true;
    remain_move -= job_list_[unit_list_[unit_index].job].consts[cells[search_position.y][search_position.x]];

    if (remain_move <= 0) {
        return;
    }

    for (auto direct : directions) {
        MapPosition position = MapPosition{search_position.x + direct.x, search_position.y + direct.y};
        FillCanMoveCells(unit_index, position, remain_move);
    }
}

/// <summary>
/// 移動可能範囲の塗りつぶし判定のセットアップ
/// </summary>
void SetupFillCanMoveCells(MapPosition start_position) {
    // カーソル位置にユニットが存在しない場合、スキップ
    int index = GetUnitIndex(start_position);
    if (index <= kUndefined) {
        return;
    }

    // 行動済みの場合、スキップ
    if (unit_list_[index].done) {
        return;
    }

    // 塗りつぶし判定初期化
    memset(fill, 0, sizeof(fill));
    for (auto direct : directions) {
        MapPosition position = MapPosition{unit_list_[index].position.x + direct.x,
                                            unit_list_[index].position.y + direct.y};
        FillCanMoveCells(index, position, unit_list_[index].move);
    }

    int tmp_index;
    for (int y = 0; y < kMapHeight; y++) {
        for (int x = 0; x < kMapWidth; x++) {
            tmp_index = GetUnitIndex(MapPosition{x, y});
            if (kUndefined < tmp_index && fill[y][x]) {
                fill[y][x] = false;
            }
        }
    }

    // 味方ユニットを選択した場合、移動先選択フェーズに移行する
    if (unit_list_[index].team == Team::kAlly) {
        selected_unit = index;
        phase_ = Phase::kSetMovePosition;
    } else {
        selected_unit = kUndefined;
    }
}

/// <summary>
/// ユニット間の距離を計算する。
/// </summary>
/// <param name="unit0_index"></param>
/// <param name="unit1_index"></param>
/// <returns></returns>
int CalculateUnitsDistance(UnitDescription unit0, UnitDescription unit1) {
    return (std::abs(unit0.position.x - unit1.position.x) + std::abs(unit0.position.y - unit1.position.y));
}

/// <summary>
/// 対象ユニットに攻撃可能か判定する。
/// </summary>
/// <param name="attack"></param>
/// <param name="defence"></param>
/// <returns></returns>
bool CanAttackUnit(UnitDescription attack, UnitDescription defence) {
    // 同じチームは範囲外扱い
    if (attack.team == defence.team) {
        return false;
    }

    return (CalculateUnitsDistance(attack, defence) <= Weapon_list_[attack.weapon].range_max);
}

/// <summary>
/// 対象ユニットに攻撃可能か判定する。
/// </summary>
/// <param name="attack"></param>
/// <param name="defence"></param>
/// <returns></returns>
bool CanAttack(int attack, int defence) {
    if (attack <= kUndefined || defence <= kUndefined) {
        return false;
    }

    UnitDescription attack_unit = unit_list_[attack];
    UnitDescription defence_unit = unit_list_[defence];

    return CanAttackUnit(attack_unit, defence_unit);
}

/// <summary>
/// 攻撃可能なユニットを取得する。
/// </summary>
/// <param name="target_index"></param>
/// <returns></returns>
int GetCanAttackUnit(int target_index) {
    int size = unit_list_.size();
    for (int i = 0; i < size; i++) {
        if (CanAttack(target_index, i)) {
            return i;
        }
    }
    return kUndefined;
}

/// <summary>
/// 戦闘クラス
/// </summary>
class BattleController {
public:
    // 攻撃種類
    enum AttackType {
        kNomal,
        kNomalResult,
        kCounter,
        kCounterResult,
        kSecond,
        kSecondResult,
        kKnockdown,

        kAttackMax,
    };

    BattleController() {
        type_ = AttackType::kAttackMax;
        attack_ = nullptr;
        defence_ = nullptr;
        second_attack_ = SecondAttack::kNothing;
        message_list_ = std::vector<std::string>();
        message_update_ = false;
    }

    ~BattleController() {
        attack_ = nullptr;
        defence_ = nullptr;
        message_list_.clear();
    }

    void Setup(UnitDescription* attack, UnitDescription* defence) {
        type_ = AttackType::kNomal;
        attack_ = attack;
        defence_ = defence;
        message_update_ = false;
        within_range_ = CanAttackUnit(*defence_, *attack_);
        Update();
    }

    bool IsAttacking() {
        return (type_ != AttackType::kAttackMax);
    }

    /// <summary>
    /// 外部公開用メッセージ
    /// </summary>
    /// <returns></returns>
    std::string Message() {
        if (!IsAttacking()) {
            return "";
        }

        std::string message = "";
        int size = message_list_.size();
        for (int i = 0; i < size; i++) {
            if (0 < i) {
                message += "\n";
            }
            message += message_list_[i];
        }

        return message;
    }

    /// <summary>
    /// 次のステートに進める
    /// </summary>
    void ProceedToTheNextState() {
        switch (type_) {
        case kNomal:
            message_update_ = false;
            type_ = AttackType::kNomalResult;
            break;

        case kNomalResult:
            message_update_ = false;
            if (defence_->hp <= kMinHp) {
                type_ = AttackType::kKnockdown;
                break;
            }

            // 射程内の場合、反撃
            if (within_range_) {
                type_ = AttackType::kCounter;
                break;
            }

            // 攻撃側が再攻撃可能か判定
            second_attack_ = JudgeSecondAttack();
            if (second_attack_ == SecondAttack::kAttack) {
                type_ = AttackType::kSecond;
                break;
            }

            CleanUp();
            break;

        case kCounter:
            message_update_ = false;
            type_ = AttackType::kCounterResult;
            break;

        case kCounterResult:
            CleanUp();

            if (attack_->hp <= 0) {
                type_ = AttackType::kKnockdown;
                break;
            }

            second_attack_ = JudgeSecondAttack();
            if (second_attack_ != SecondAttack::kNothing) {
                type_ = AttackType::kSecond;
            }
            break;

        case kSecond:
            message_update_ = false;
            type_ = AttackType::kSecondResult;
            break;

        case kSecondResult:
            CleanUp();
            if (attack_->hp <= 0 || defence_->hp <= 0) {
                type_ = AttackType::kKnockdown;
                break;
            }
            break;

        case kKnockdown:
            message_update_ = false;
            second_attack_ = SecondAttack::kNothing;
            type_ = AttackType::kAttackMax;
            CleanUp();
            break;

        case kAttackMax:
        default:
            break;
        }

        Update();
    }

private:
    // 再攻撃する側
    enum SecondAttack {
        kNothing,
        kAttack,
        kDefence,

        kSecondAttackMax,
    };

    /// <summary>
    /// クリティカルが発生するか確認する。
    /// </summary>
    /// <param name="attack"></param>
    /// <returns></returns>
    bool IsCritical(UnitDescription* attack) {
        int critical = (attack->skill + attack->luck) / 2 + Weapon_list_[attack->weapon].critical;

        std::uniform_int_distribution<> dist(0, 100);
        int random = dist(randomizer_);

        return (random < critical);
    }

    /// <summary>
    /// 攻撃が命中したか確認する。
    /// </summary>
    /// <param name="attack"></param>
    /// <param name="defence"></param>
    /// <returns></returns>
    bool IsHit(UnitDescription* attack, UnitDescription* defence) {
        int hit = attack->skill + Weapon_list_[attack->weapon].hit;
        int parry = defence->agility - Weapon_list_[defence->weapon].weight;
        parry += cell_list_[cells[defence->position.y][defence->position.x]].defence;

        hit -= parry;

        std::uniform_int_distribution<> dist(0, 100);
        int random = dist(randomizer_);

        // 最終命中率より乱数が小さい場合、命中
        return (random < hit);
    }

    /// <summary>
    /// ダメージ計算
    /// </summary>
    /// <param name="attack"></param>
    /// <param name="defence"></param>
    /// <returns></returns>
    int CalculateDamage(UnitDescription* attack, UnitDescription* defence, bool critical) {
        int damage = attack->strength + Weapon_list_[attack->weapon].power;

        damage -= defence->defence;

        if (critical) {
            damage *= kCriticalCorrection;
        }

        return max(damage, 0);
    }

    /// <summary>
    /// 再攻撃判定を算出する。
    /// </summary>
    /// <returns></returns>
    SecondAttack JudgeSecondAttack() {
        int attack_speed = attack_->agility - Weapon_list_[attack_->weapon].weight;
        int defence_speed = defence_->agility - Weapon_list_[defence_->weapon].weight;

        if (defence_speed < attack_speed) {
            return SecondAttack::kAttack;
        }

        if (attack_speed < defence_speed) {
            return SecondAttack::kDefence;
        }

        return SecondAttack::kNothing;
    }

    /// <summary>
    /// ダメージ計算やそれに伴うメッセージの更新を行う
    /// </summary>
    void Update() {
        if (message_update_) {
            return;
        }

        message_list_.clear();
        switch (type_) {
        case kNomal:
            message_list_.push_back(attack_->name + "の攻撃!");
            break;

        case kNomalResult:
            message_list_.push_back(attack_->name + "の攻撃!");
            UpdateDamageResult(attack_, defence_);
            break;

        case kCounter:
            message_list_.push_back(defence_->name + "の反撃!");
            break;

        case kCounterResult:
            message_list_.push_back(defence_->name + "の反撃!");
            UpdateDamageResult(defence_, attack_);
            break;

        case kSecond:
            if (second_attack_ == SecondAttack::kAttack) {
                message_list_.push_back(attack_->name + "の再攻撃!");
            } else {
                message_list_.push_back(defence_->name + "の再攻撃!");
            }
            break;

        case kSecondResult:
            if (second_attack_ == SecondAttack::kAttack) {
                message_list_.push_back(attack_->name + "の再攻撃!");
                UpdateDamageResult(attack_, defence_);
            } else {
                message_list_.push_back(defence_->name + "の再攻撃!");
                UpdateDamageResult(defence_, attack_);
            }
            break;

        case kKnockdown:
            if (attack_->hp <= 0) {
                message_list_.push_back(GetKnockdownMessage(attack_->name, attack_->team));
                UnitDeath(attack_);
            } else {
                message_list_.push_back(GetKnockdownMessage(defence_->name, defence_->team));
                UnitDeath(defence_);
            }
            break;

        case kAttackMax:
        default:
            break;
        }

        message_update_ = true;
    }

    /// <summary>
    /// ユニット死亡時のメッセージ
    /// </summary>
    /// <param name="unit_name"></param>
    /// <param name="team"></param>
    /// <returns></returns>
    std::string GetKnockdownMessage(std::string unit_name, Team team) {
        if (team == Team::kAlly) {
            return unit_name + "が倒された…";
        } else {
            return unit_name + "を倒した!";
        }
    }

    /// <summary>
    /// ダメージ計算結果に関わるメッセージを更新する。
    /// </summary>
    /// <param name="attack_unit"></param>
    /// <param name="defence_unit"></param>
    void UpdateDamageResult(UnitDescription* attack_unit, UnitDescription* defence_unit) {
        // クリティカルは必中
        bool critical = IsCritical(attack_unit);
        if (critical) {
            message_list_.push_back("必殺の一撃!");
        } else if (!IsHit(attack_unit, defence_unit)) {
            message_list_.push_back(defence_unit->name + "は素早く身をかわした!");
            return;
        }

        const int kNodamage = 0;
        int damage = CalculateDamage(attack_unit, defence_unit, critical);
        if (damage <= kNodamage) {
            message_list_.push_back("ダメージを与えられない!");
        } else {
            message_list_.push_back(defence_unit->name + "に" + std::to_string(damage) + "のダメージ!");
            defence_unit->hp -= damage;
        }
    }

    /// <summary>
    /// ユニット死亡処理
    /// </summary>
    /// <param name="unit"></param>
    void UnitDeath(UnitDescription* unit) {
        unit->position.x = -1;
        unit->position.y = -1;
    }

    /// <summary>
    /// バトル終了時のクリーンアップ
    /// </summary>
    void CleanUp() {
        message_update_ = false;
        second_attack_ = SecondAttack::kNothing;
        type_ = AttackType::kAttackMax;
    }

    // クリティカル発生時のダメージ補正(n倍)
    const int kCriticalCorrection = 3;

    SecondAttack second_attack_;

    AttackType type_;
    UnitDescription* attack_;
    UnitDescription* defence_;

    std::vector<std::string> message_list_;
    bool message_update_;

    // 反撃側の攻撃範囲内判定
    bool within_range_;
};
BattleController* battle_controller_;

/// <summary>
/// 敵ユニット行動クラス
/// </summary>
class EnemyBehavior {
public:
    EnemyBehavior(UnitDescription* unit) :
        unit_(unit) {
        target_position_ = {kUndefined, kUndefined};
    }
    ~EnemyBehavior() {
        unit_ = nullptr;
    }

    /// <summary>
    /// 移動目標地点を設定する。
    /// </summary>
    /// <param name="target_position"></param>
    void Setup(std::vector<UnitDescription>& unit_list) {
        remain_move_ = unit_->move;

        if (unit_->name == "ガザック") {
            // 城に向かう
            target_position_.x = 4;
            target_position_.y = 4;
            return;
        }

        // 雑魚はマルスに向かう
        for (auto unit : unit_list) {
            if (unit.job != Job::kLoad) {
                continue;
            }
            target_position_ = unit.position;
            break;
        }
    }

    /// <summary>
    /// 敵ユニットを行動させる。
    /// </summary>
    void Update() {
        if (IsEnd()) {
            return;
        }

        int attack_index = GetUnitIndex(unit_->position);
        if (ProceedBattlePhase(attack_index)) {
            return;
        }

        // 移動可能な地点に1マス分移動
        Move(attack_index);

        remain_move_ = CalculateRemainMove(remain_move_);

        if (remain_move_ == 0) {
            unit_->done = true;
        }
    }

    bool IsCanMove(int search_index, MapPosition next_position, int remain_move) {
        // 移動可能なマップか判定
        if (!CanMove(search_index, next_position, remain_move)) {
            return false;
        }

        // 移動先に味方ユニットがいない場合、移動可能
        int unit_index = GetUnitIndex(next_position);
        return (unit_index == kUndefined);
    }

    /// <summary>
    /// ユニット情報を取得。
    /// </summary>
    UnitDescription* GetUnit() const {
        return unit_;
    }

    /// <summary>
    /// 行動済み判定を取得する。
    ///  死亡時はtrue
    /// </summary>
    /// <returns></returns>
    bool IsEnd() {
        return unit_->done || unit_->hp <= kMinHp;
    }

private:
    /// <summary>
    /// 射程範囲内にプレイヤーのユニットがいる場合、バトルに移行する。
    /// </summary>
    /// <param name="attack_index"></param>
    /// <returns></returns>
    bool ProceedBattlePhase(int attack_index) {
        int deffence_index = GetCanAttackUnit(attack_index);
        if (kUndefined == deffence_index) {
            return false;
        }

        if (!CanAttack(attack_index, deffence_index)) {
            return false;
        }

        battle_controller_->Setup(unit_, &unit_list_[deffence_index]);
        unit_->done = true;
        phase_ = Phase::kSelectUnit;
        return true;
    }

    /// <summary>
    /// ユニットを移動させる。
    /// </summary>
    /// <param name="attack_index"></param>
    void Move(int attack_index) {
        std::vector<MapPosition> next_position_list = {
            MapPosition{CalculateGreaterOrLesser(unit_->position.x, target_position_.x), unit_->position.y},
            MapPosition{unit_->position.x, CalculateGreaterOrLesser(unit_->position.y, target_position_.y)},
        };

        for (auto position : next_position_list) {
            // 移動できない場合、スキップする。
            if (position.x == unit_->position.x && position.y == unit_->position.y) {
                continue;
            }

            if (IsCanMove(attack_index, position, remain_move_)) {
                unit_->position = position;
                return;
            }
        }

        // 行き止まりの場合
        remain_move_ = 0;
    }

    /// <summary>
    /// 移動方向を計算する。
    /// </summary>
    /// <param name="base"></param>
    /// <param name="target"></param>
    /// <returns></returns>
    int CalculateGreaterOrLesser(int base, int target) {
        if (base < target) {
            base++;
        } else if (base > target) {
            base--;
        }
        return base;
    }

    /// <summary>
    /// 残り移動力を計算する。
    /// </summary>
    /// <param name="remain_move"></param>
    /// <returns></returns>
    int CalculateRemainMove(int remain_move) {
        JobDescription job = job_list_[unit_->job];
        int cell = cells[unit_->position.y][unit_->position.x];
        int cost = job.consts[cell];
        int result = max(remain_move - cost, 0);
        return result;
    }

private:
    int remain_move_;
    UnitDescription* unit_;
    MapPosition target_position_;
};

/// <summary>
/// 敵ユニット行動管理クラス
/// </summary>
class EnemyBehaviorManager {
public:
    EnemyBehaviorManager(std::vector<UnitDescription>& unit_list) {
        unit_list_ = std::vector<EnemyBehavior*>();

        for (auto& unit : unit_list) {
            if (unit.team == Team::kAlly) {
                continue;
            }
            unit_list_.push_back(new EnemyBehavior(&(unit)));
        }

        target_position_ = {kUndefined, kUndefined};
    }
    ~EnemyBehaviorManager() {
        unit_list_.clear();
    }

    /// <summary>
    /// 移動目標地点を設定する。
    /// </summary>
    /// <param name="unit_list"></param>
    void Setup(std::vector<UnitDescription>& unit_list) {
        for (auto unit : unit_list_) {
            if (unit->IsEnd()) {
                continue;
            }
            unit->Setup(unit_list);
        }
    }

    /// <summary>
    /// 敵ユニットを行動させる。
    /// </summary>
    void Update() {
        EnemyBehavior* unit = GetBehaviorUnit();
        if (unit == nullptr) {
            return;
        }
        unit->Update();
    }

    /// <summary>
    /// 行動中のユニットを取得する。
    /// </summary>
    /// <returns></returns>
    UnitDescription* GetUnit() const {
        for (auto unit : unit_list_) {
            if (unit->IsEnd()) {
                continue;
            }
            return unit->GetUnit();
        }

        return nullptr;
    }

    /// <summary>
    /// 全ユニットが行動済みか確認する。
    /// </summary>
    /// <returns></returns>
    bool IsEnd() {
        EnemyBehavior* unit = GetBehaviorUnit();
        return (unit == nullptr);
    }

private:
    /// <summary>
    /// 行動するユニットを取得する。
    /// </summary>
    /// <returns></returns>
    EnemyBehavior* GetBehaviorUnit() {
        for (auto unit : unit_list_) {
            if (unit->IsEnd()) {
                continue;
            }
            return unit;
        }

        return nullptr;
    }

    std::vector<EnemyBehavior*> unit_list_;
    MapPosition target_position_;
};
EnemyBehaviorManager* enemy_behavior_manager_;

/// <summary>
/// カーソル座標と一致するか確認
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>true：一致、false：不一致</returns>
bool IsMatchCursorPosition(int x, int y) {
    if (cursor_position.x != x) {
        return false;
    }
    if (cursor_position.y != y) {
        return false;
    }
    return true;
}

/// <summary>
/// /マップ表示
/// </summary>
/// <returns></returns>
std::string DisplayMap() {
    std::string draw_map;
    for (int y = 0; y < kMapHeight; y++) {
        for (int x = 0; x < kMapWidth; x++) {
            if (IsMatchCursorPosition(x, y)) {
                draw_map += "◎";
                continue;
            }

            if (fill[y][x]) {
                draw_map += "■";
                continue;
            }

            int index = GetUnitIndex(MapPosition{x, y});
            if (kUndefined < index) {
                draw_map += job_list_[unit_list_[index].job].aa;
            } else {
                draw_map += cell_list_[cells[y][x]].aa;
            }
        }
        draw_map += "\n";
    }

    return draw_map;
}

/// <summary>
/// 描画関連集約処理
/// </summary>
/// <returns></returns>
std::string Draw() {
    std::string message = DisplayMap();

    message += DisplayPhaseGuidance(phase_);
    if (phase_ == Phase::kStartAlly || phase_ == Phase::kStartEnemy) {
        return message;
    }

    if (battle_controller_->IsAttacking()) {
        message += battle_controller_->Message();
        return message;
    }

    MapPosition unit_position;
    if (turn_ == Turn::kAllyTurn) {
        unit_position = cursor_position;
    } else {
        UnitDescription* unit = enemy_behavior_manager_->GetUnit();
        unit_position = (unit == nullptr) ? MapPosition{kUndefined, kUndefined} : unit->position;
    }

    int unit_index = GetUnitIndex(unit_position);
    if (kUndefined < unit_index) {
        message += DisplayUnitParameter(unit_index);
    } else {
        message += DisplayCellParameter(cursor_position);
    }

    return message;
}

/// <summary>
/// カーソル移動処理
/// </summary>
/// <param name="input_param"></param>
void MoveCursor(WPARAM input_param) {
    AtlTrace("KeyUp = %Xh\n", input_param);

    if (phase_ == Phase::kStartAlly || phase_ == Phase::kStartEnemy) {
        switch (input_param) {
            // Enterキー押下時
        case '\r':
        {
            if (phase_ == Phase::kStartAlly) {
                phase_ = Phase::kSelectUnit;
            } else {
                phase_ = Phase::kPhaseMax;
            }
        }
        }
        return;
    }

    // 攻撃中はカーソル移動等を行わない
    if (battle_controller_->IsAttacking()) {
        switch (input_param) {
            // Enterキー押下時
        case '\r':
        {
            battle_controller_->ProceedToTheNextState();
        }
        }
        return;
    }

    if (turn_ == Turn::kEnemyTurn) {
        switch (input_param) {
            // Enterキー押下時
        case '\r':
        {
            enemy_behavior_manager_->Update();

            // TODO:ここの判定前に、バトルに移行したかの判定が必要?
            if (enemy_behavior_manager_->IsEnd()) {
                phase_ = Phase::kStartAlly;
                turn_ = Turn::kAllyTurn;
                ResetDone(Team::kAlly);
            }
        }
        }
        return;
    }

    switch (input_param) {
    case 'w': cursor_position.y--; break;
    case 's': cursor_position.y++; break;
    case 'a': cursor_position.x--; break;
    case 'd': cursor_position.x++; break;

        // Enterキー押下時
    case '\r':
    {
        switch (phase_) {
        case Phase::kSelectUnit:
        {
            SetupFillCanMoveCells(cursor_position);
            break;
        }
        case Phase::kSetMovePosition:
            if (fill[cursor_position.y][cursor_position.x]) {
                unit_list_[selected_unit].position = cursor_position;
                memset(fill, 0, sizeof(fill));

                if (kUndefined < GetCanAttackUnit(selected_unit)) {
                    phase_ = Phase::kSelectAttackUnit;
                } else {
                    unit_list_[selected_unit].done = true;
                    phase_ = Phase::kSelectUnit;
                }
            }
            break;

        case Phase::kSelectAttackUnit:
        {
            if (    cursor_position.x == unit_list_[selected_unit].position.x
                &&  cursor_position.y == unit_list_[selected_unit].position.y) {
                unit_list_[selected_unit].done = true;
                phase_ = Phase::kSelectUnit;
                break;
            }

            int index = GetUnitIndex(cursor_position);
            if (CanAttack(selected_unit, index)) {
                battle_controller_->Setup(&unit_list_[selected_unit], &unit_list_[index]);
                unit_list_[selected_unit].done = true;
                phase_ = Phase::kSelectUnit;
            }
            break;
        }
        }
        break;
    }

    case 'e':
    {
        phase_ = Phase::kStartEnemy;
        turn_ = Turn::kEnemyTurn;
        ResetDone(Team::kEnemy);
        enemy_behavior_manager_->Setup(unit_list_);
        break;
    }
    }

    // 画面端を超えた場合、反対にワープする
    cursor_position.x = (kMapWidth + cursor_position.x) % kMapWidth;
    cursor_position.y = (kMapHeight + cursor_position.y) % kMapHeight;
}

/// <summary>
/// アプリケーションウィンドウのメッセージハンドラ
/// </summary>
/// <param name="hWnd"></param>
/// <param name="msg"></param>
/// <param name="wParam"></param>
/// <param name="lParam"></param>
/// <returns></returns>
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    std::string draw_map = Draw();

    HDC hdc;
    PAINTSTRUCT paintstruct;
    RECT rect;
    switch (msg) {
        //escキーを押したときの自沈処理
    case WM_KEYUP:
        break;

        //クローズボタンをクリックしたときの自沈処理
    case WM_DESTROY:
        PostQuitMessage(0);
        return (0);
        break;

        // ウィンドウが再びアクティブになりました
    case WM_ACTIVATE:
        break;

        //タイマ割り込み
        // ウィンドウがアクティブな場合だけ、ゲーム本体の処理を行う。
    case WM_TIMER:
        break;

    case WM_LBUTTONDOWN:
        hdc = GetDC(hWnd);
        return 0;

        //----ペイント----
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &paintstruct);

        SetBkColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, OPAQUE);
        SetTextColor(hdc, RGB(255, 255, 255));

        // TODO:一先ず描画領域の上書きにより、文字削除
        SelectObject(hdc, GetStockObject(BLACK_BRUSH));
        Rectangle(hdc, 0, 0, kWindowWidth, kWindowHeight);

        rect.top = kWindowTop;
        rect.left = kWindowLeft;
        rect.right = kWindowWidth;
        rect.bottom = kWindowHeight;

        DrawText(hdc,
                 draw_map.c_str(),
                 -1,
                 &rect,
                 DT_WORDBREAK);

        EndPaint(hWnd, &paintstruct);
        break;

    case WM_CHAR:
        MoveCursor(wParam);
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    default:
        break;
    }

    return(DefWindowProc(hWnd, msg, wParam, lParam));
}

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT) {
    srand((unsigned) time(NULL));	//ランダマイズ（乱数の初期化）

    //多重起動のチェック
    HANDLE hMutex = CreateMutex(NULL, FALSE, kWindowTitle);
    DWORD theErr = GetLastError();
    if (theErr == ERROR_ALREADY_EXISTS) {
        //多重起動している
        if (hMutex) {
            CloseHandle(hMutex);
        }
        MessageBox(NULL, "既に起動中です。", kErrorMessage, MB_OK | MB_ICONHAND);
        return(FALSE);
    }

    //ウィンドウクラスの設定
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        MsgProc,
        0L,
        0L,
        GetModuleHandle(NULL),
        NULL,
        NULL,
        NULL,
        NULL,
        kWindowTitle,
        NULL
    };

    //コケたら帰る
    if (!RegisterClassEx(&wc)) {
        return(FALSE);
    }

    // メルセンヌ・ツイスター法による擬似乱数生成器を、
    // ハードウェア乱数をシードにして初期化
    std::random_device seed_gen;
    randomizer_ = std::mt19937(seed_gen());

    // 最大化ボタンを持たない・境界変更のできないwindow
    DWORD window_style = ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
    window_style &= WS_OVERLAPPEDWINDOW;

    const int kFirstPirateIndex = 10;
    int pirate_count = 0;
    for (int y = 0; y < kMapHeight; y++) {
        for (int x = 0; x < kMapWidth; x++) {
            char c = map_data_[(y * kMapWidth) + x];
            if (isdigit(c)) {
                // 文字を数値に変換
                cells[y][x] = c - '0';
            } else {
                // ユニットの初期座標設定
                // 'k' = 雑魚
                // 'a' = マルス
                int index;
                if (c == 'k') {
                    index = kFirstPirateIndex + pirate_count;
                    pirate_count++;
                } else {
                    index = (c - 'a');
                }
                unit_list_[index].position = MapPosition{x, y};
                unit_list_[index].hp = unit_list_[index].max_hp;
                unit_list_[index].done = false;
                cells[y][x] = Cell::kPlane;
            }
        }
    }

    AtlTrace("kPirate = %d", pirate_count);

    // クラス生成
    battle_controller_ = new BattleController();
    enemy_behavior_manager_ = new EnemyBehaviorManager(unit_list_);

    //アプリケーションウィンドウの生成
    HWND hWnd = CreateWindow(kWindowTitle,
                             kWindowTitle,
                             window_style,
                             kWindowLeft,
                             kWindowTop,
                             kWindowLeft + kWindowWidth,
                             kWindowTop + kWindowHeight,
                             GetDesktopWindow(),
                             NULL,
                             wc.hInstance,
                             NULL);

    // ウィンドウの表示
    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    // タイマをセットする。
    SetTimer(hWnd, 0, kTimer, NULL);

    // メッセージループ
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        // イベントの取得
        if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            // CPU 不可解消のため
            Sleep(kSleep);
            continue;
        }

        // ここでイベントを取得する。
        if (!GetMessage(&msg, NULL, 0, 0)) {
            msg.message = WM_QUIT;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // クラス後始末
    delete(battle_controller_);

    return (0);
}
