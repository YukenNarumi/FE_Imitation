#include <algorithm>
#include <atlbase.h>
#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <time.h>

namespace {
// �P�ʎ��Ԃ�����̈ړ��ʐݒ�
const constexpr int kDivUnit = 45;
const constexpr int kTimer = (1000 / kDivUnit);
const constexpr int kSleep = 16;

// TODO:��^��
static const constexpr char kWindowTitle[] = "FE���ǂ�";
static const constexpr char kErrorMessage[] = "�G���[";


/*�E�B���h�E�̌`�Ȃ�*/
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
    { "�C",      "�`",   30, false},
    { "����",    "�@",     5, false},
    { "��",      "��",   15, false},
    { "�R",      "��",    25, false},
    { "���R",    "��",     0, false},
    { "��",      "��",    0, false},
    { "��",      "��",    20, false},
    { "��",      "��",    30, false},
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

// �E��
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
    { "���[�h",     "�N", (-1, 1, 2,  4, -1, 1, 2, 2) },
    { "�p���f�B��", "�R", (-1, 1, 3,  6, -1, 1, 2, 2) },
    { "S�i�C�g",    "�R", (-1, 1, 3, -1, -1, 1, 2, 2) },
    { "A�i�C�g",    "�d", (-1, 1, 2, -1, -1, 1, 2, 2) },
    { "�A�[�`���[", "��", (-1, 1, 3, -1, -1, 1, 2, 2) },
    { "P�i�C�g",    "�V", (1, 1, 1,  1,  1, 1, 1, 1) },
    { "�C��",       "�C", (2, 1, 2,  4, -1, 1, 2, 2) },
    { "�n���^�[",   "��", (-1, 1, 2,  3, -1, 1, 2, 2) },
    { "����",       "��", (-1, 1, 2,  4, -1, 1, 2, 2) }
};

// ����
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
    { "�S��",    5, 2, 100,  0, 1, 1 },
    { "�h�ˌ�",  5, 1, 100, 10, 1, 1 },
    { "��",      8, 6,  80,  0, 1, 1 },
    { "�⑄",   12, 7,  80,  0, 1, 1 },
    { "�葄",    7, 6,  70,  0, 1, 2 },
    { "�؋|",    4, 1,  90,  0, 2, 2 },
    { "�|�|",    7, 3,  80,  0, 2, 2 },
    { "�W",      5, 2, 100, 20, 2, 2 },
    { "��",      7, 7,  80,  0, 1, 1 },
    { "�|��",    9, 9,  70,  0, 1, 1 },
};

// ���j�b�g
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
    { "�}���X",     Job::kLoad,    18, 5,  3,  5,  7, 7,  7,  7, Weapon::kRapier },
    { "�W�F�C�K��", Job::kParadin, 20, 7, 10, 10,  8, 1,  9, 10, Weapon::kIronSword },
    { "�J�C��",     Job::kSKnight, 18, 7,  5,  5,  6, 3,  7,  9, Weapon::kSpear },
    { "�A�x��",     Job::kSKnight, 18, 6,  7,  6,  7, 2,  7,  9, Weapon::kIronSword },
    { "�h�[�K",     Job::kAKnight, 16, 7,  3,  4,  3, 1, 11,  5, Weapon::kIronSword },
    { "�S�[�h��",   Job::kArcher,  16, 5,  1,  5,  4, 4,  6,  5, Weapon::kCrossBow },
    { "�V�[�_",     Job::kPKnight, 16, 3,  6,  7, 12, 9,  7,  8, Weapon::kIronSword },
    { "�K�U�b�N",   Job::kPirate,  24, 7,  3,  7,  8, 0,  6,  6, Weapon::kStealAxe },
    { "�K���_��",   Job::kHunter,  18, 6,  1,  5,  5, 0,  3,  6, Weapon::kBow },
    { "�K���_��",   Job::kThief,   16, 3,  1,  2,  9, 0,  2,  7, Weapon::kIronSword },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Weapon::kAxe },
};

/// <summary>
/// �w����W�̃��j�b�g�C���f�b�N�X�擾
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
/// �A�v���P�[�V�����E�B���h�E�̃��b�Z�[�W�n���h��
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
        //esc�L�[���������Ƃ��̎�������
    case WM_KEYUP:
        break;

        //�N���[�Y�{�^�����N���b�N�����Ƃ��̎�������
    case WM_DESTROY:
        PostQuitMessage(0);
        return (0);
        break;

        // �E�B���h�E���ĂуA�N�e�B�u�ɂȂ�܂���
    case WM_ACTIVATE:
        break;

        //�^�C�}���荞��
        // �E�B���h�E���A�N�e�B�u�ȏꍇ�����A�Q�[���{�̂̏������s���B
    case WM_TIMER:
        break;

    case WM_LBUTTONDOWN:
        hdc = GetDC(hWnd);
        //TextOut(hdc_, 10, 10, draw_map.c_str(), lstrlen(draw_map.c_str()));
        return 0;

        //----�y�C���g----
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &paintstruct);

        SetTextColor(hdc, RGB(0, 128, 255));
        rect.top = kWindowTop;
        rect.left = kWindowLeft;
        rect.right = kWindowWidth;
        rect.bottom = kWindowHeight;
        //DrawText(hdc, "�g�p�� sample\n�v���t�B�b�N�X(&A)", -1, &rect, DT_WORDBREAK | DT_CENTER);
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
    srand((unsigned) time(NULL));	//�����_�}�C�Y�i�����̏������j

    //���d�N���̃`�F�b�N
    HANDLE hMutex = CreateMutex(NULL, FALSE, kWindowTitle);
    DWORD theErr = GetLastError();
    if (theErr == ERROR_ALREADY_EXISTS) {
        //���d�N�����Ă���
        if (hMutex) {
            CloseHandle(hMutex);
        }
        MessageBox(NULL, "���ɋN�����ł��B", kErrorMessage, MB_OK | MB_ICONHAND);
        return(FALSE);
    }

    //�E�B���h�E�N���X�̐ݒ�
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

    //�R�P����A��
    if (!RegisterClassEx(&wc)) {
        return(FALSE);
    }

    // �ő剻�{�^���������Ȃ��E���E�ύX�̂ł��Ȃ�window
    DWORD window_style = ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
    window_style &= WS_OVERLAPPEDWINDOW;

    const int kFirstPirateIndex = 10;
    int pirate_count = 0;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char c = map_data_[(y * MAP_WIDTH) + x];
            if (isdigit(c)) {
                // �����𐔒l�ɕϊ�
                cells[y][x] = c - '0';
            } else {
                // ���j�b�g�̏������W�ݒ�
                // 'k' = �G��
                // 'a' = �}���X
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

    //�A�v���P�[�V�����E�B���h�E�̐���
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

    // �E�B���h�E�̕\��
    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    // �^�C�}���Z�b�g����B
    SetTimer(hWnd, 0, kTimer, NULL);

    // ���b�Z�[�W���[�v
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        // �C�x���g�̎擾
        if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            // CPU �s�����̂���
            Sleep(kSleep);
            continue;
        }

        // �����ŃC�x���g���擾����B
        if (!GetMessage(&msg, NULL, 0, 0)) {
            msg.message = WM_QUIT;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (0);
}
