#include <algorithm>
#include <atlbase.h>
#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <time.h>

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
}

struct MapPosition {
    int x;
    int y;
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

const int MAP_WIDTH = 30;
const int MAP_HEIGHT = 13;
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
int cells[MAP_HEIGHT][MAP_WIDTH];

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
    Weapon weapon;
    MapPosition position;
};
std::vector<UnitDescription> unit_list_ = {
    { "マルス",     Job::kLoad,    18, 5,  3,  5,  7, 7,  7,  7, Weapon::kRapier },
    { "ジェイガン", Job::kParadin, 20, 7, 10, 10,  8, 1,  9, 10, Weapon::kIronSword },
    { "カイン",     Job::kSKnight, 18, 7,  5,  5,  6, 3,  7,  9, Weapon::kSpear },
    { "アベル",     Job::kSKnight, 18, 6,  7,  6,  7, 2,  7,  9, Weapon::kIronSword },
    { "ドーガ",     Job::kAKnight, 16, 7,  3,  4,  3, 1, 11,  5, Weapon::kIronSword },
    { "ゴードン",   Job::kArcher,  16, 5,  1,  5,  4, 4,  6,  5, Weapon::kCrossBow },
    { "シーダ",     Job::kPKnight, 16, 3,  6,  7, 12, 9,  7,  8, Weapon::kIronSword },
    { "ガザック",   Job::kPirate,  24, 7,  3,  7,  8, 0,  6,  6, Weapon::kStealAxe },
    { "ガルダ兵",   Job::kHunter,  18, 6,  1,  5,  5, 0,  3,  6, Weapon::kBow },
    { "ガルダ兵",   Job::kThief,   16, 3,  1,  2,  9, 0,  2,  7, Weapon::kIronSword },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "ガルダ兵",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
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
/// アプリケーションウィンドウのメッセージハンドラ
/// </summary>
/// <param name="hWnd"></param>
/// <param name="msg"></param>
/// <param name="wParam"></param>
/// <param name="lParam"></param>
/// <returns></returns>
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    std::string draw_map;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            int index = GetUnitIndex(MapPosition{x, y});
            if (kUndefined < index) {
                draw_map += job_list_[unit_list_[index].job].aa;
            } else {
                draw_map += cell_list_[cells[y][x]].aa;
            }
        }
        draw_map += "\n";
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
        //TextOut(hdc_, 10, 10, draw_map.c_str(), lstrlen(draw_map.c_str()));
        return 0;

        //----ペイント----
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &paintstruct);

        SetTextColor(hdc, RGB(0, 128, 255));
        rect.top = kWindowTop;
        rect.left = kWindowLeft;
        rect.right = kWindowWidth;
        rect.bottom = kWindowHeight;
        //DrawText(hdc, "使用例 sample\nプリフィックス(&A)", -1, &rect, DT_WORDBREAK | DT_CENTER);
        DrawText(hdc,
                 draw_map.c_str(),
                 -1,
                 &rect,
                 DT_WORDBREAK);

        EndPaint(hWnd, &paintstruct);
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
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char c = map_data_[(y * MAP_WIDTH) + x];
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
