#include <algorithm>
#include <atlbase.h>
#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <time.h>
#include <sstream>
#include <iomanip>

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
}

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
    kSelectUnit,
    kSetMovePosition,
    kSelectAttackUnit,

    kPhaseMax,
};
Phase phase_ = Phase::kSelectUnit;

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
    { "ロード",     "君", (-1, 1, 2,  4, -1, 1, 2, 2) },
    { "パラディン", "騎", (-1, 1, 3,  6, -1, 1, 2, 2) },
    { "Sナイト",    "騎", (-1, 1, 3, -1, -1, 1, 2, 2) },
    { "Aナイト",    "重", (-1, 1, 2, -1, -1, 1, 2, 2) },
    { "アーチャー", "射", (-1, 1, 3, -1, -1, 1, 2, 2) },
    { "Pナイト",    "天", (1, 1, 1,  1,  1, 1, 1, 1) },
    { "海賊",       "海", (2, 1, 2,  4, -1, 1, 2, 2) },
    { "ハンター",   "狩", (-1, 1, 2,  3, -1, 1, 2, 2) },
    { "盗賊",       "盗", (-1, 1, 2,  4, -1, 1, 2, 2) }
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
    parameter += unit_list_[index].name + "\n";
    parameter += job_list_[unit_list_[index].job].name + "\n";

    const int kDigit = 2;
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
    std::string result;
    switch (phase_) {
    case kSelectUnit:       result = "ユニットを選択してください。";  break;
    case kSetMovePosition:  result = "移動先を設定してください。";  break;
    case kSelectAttackUnit: result = "攻撃対象を選んでください。";  break;
    default:
        break;
    }
    result += "\n\n";
    return result;
}

// カーソル
MapPosition cursor_position{0, 0};

/// <summary>
/// 移動可能範囲の塗りつぶし判定更新
/// </summary>
/// <param name="unit_index"></param>
/// <param name="position"></param>
/// <param name="remain_move"></param>
void FillCanMoveCells(int unit_index, MapPosition position, int remain_move) {
    if (position.x < 0 || kMapWidth <= position.x) {
        return;
    }

    if (position.y < 0 || kMapHeight <= position.y) {
        return;
    }

    int search_unit_index = GetUnitIndex(position);
    if (kUndefined < search_unit_index && unit_list_[unit_index].team != unit_list_[search_unit_index].team) {
        return;
    }

    fill[position.y][position.x] = true;
}

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
/// カーソル移動処理
/// </summary>
/// <param name="input_param"></param>
void MoveCursor(WPARAM input_param) {
    AtlTrace("KeyUp = %Xh\n", input_param);

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
            // カーソル位置にユニットが存在しない場合、スキップ
            int index = GetUnitIndex(cursor_position);
            if (index <= kUndefined) {
                break;
            }
            // 塗りつぶし判定初期化
            memset(fill, 0, sizeof(fill));
            for (auto direct : directions) {
                MapPosition position = MapPosition{ unit_list_[index].position.x + direct.x,
                                                    unit_list_[index].position.y + direct.y };
                FillCanMoveCells(index, position, unit_list_[index].move);
            }
            break;
        }
        case Phase::kSetMovePosition:
            break;

        case Phase::kSelectAttackUnit:
            break;
        }
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

    draw_map += DisplayPhaseGuidance(phase_);

    int unit_index = GetUnitIndex(MapPosition{cursor_position.x, cursor_position.y});
    if (kUndefined < unit_index) {
        draw_map += DisplayUnitParameter(unit_index);
    } else {
        draw_map += DisplayCellParameter(cursor_position);
    }

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
                cells[y][x] = Cell::kPlane;
            }
        }
    }

    AtlTrace("kPirate = %d", pirate_count);

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

    return (0);
}
