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

// �}�b�v�T�C�Y
const constexpr int kMapWidth = 30;
const constexpr int kMapHeight = 13;
}

struct MapPosition {
    int x;
    int y;
};

// x,y�����̈ړ���
struct MoveVector {
    int x;
    int y;
};

// �Q�[���̃t�F�[�Y
enum Phase {
    kSelectUnit,
    kSetMovePosition,
    kSelectAttackUnit,

    kPhaseMax,
};
Phase phase_ = Phase::kSelectUnit;

// ���p
enum Direction {
    kNorth,
    kWest,
    kSouth,
    kEast,

    kDirectionMax,
};

// �e���p�̈ړ���
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
    { "�C",      "�`",   30, false},
    { "����",    "�@",     5, false},
    { "��",      "��",   15, false},
    { "�R",      "��",    25, false},
    { "���R",    "��",     0, false},
    { "��",      "��",    0, false},
    { "��",      "��",    20, false},
    { "��",      "��",    30, false},
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

// �ړ���h��Ԃ�����p
bool fill[kMapHeight][kMapWidth];

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

enum Team {
    kAlly,
    kEnemy,

    kTeamMax,
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
    Team team;
    Weapon weapon;
    MapPosition position;
};
std::vector<UnitDescription> unit_list_ = {
    { "�}���X",     Job::kLoad,    18, 5,  3,  5,  7, 7,  7,  7, Team::kAlly,  Weapon::kRapier },
    { "�W�F�C�K��", Job::kParadin, 20, 7, 10, 10,  8, 1,  9, 10, Team::kAlly,  Weapon::kIronSword },
    { "�J�C��",     Job::kSKnight, 18, 7,  5,  5,  6, 3,  7,  9, Team::kAlly,  Weapon::kSpear },
    { "�A�x��",     Job::kSKnight, 18, 6,  7,  6,  7, 2,  7,  9, Team::kAlly,  Weapon::kIronSword },
    { "�h�[�K",     Job::kAKnight, 16, 7,  3,  4,  3, 1, 11,  5, Team::kAlly,  Weapon::kIronSword },
    { "�S�[�h��",   Job::kArcher,  16, 5,  1,  5,  4, 4,  6,  5, Team::kAlly,  Weapon::kCrossBow },
    { "�V�[�_",     Job::kPKnight, 16, 3,  6,  7, 12, 9,  7,  8, Team::kAlly,  Weapon::kIronSword },
    { "�K�U�b�N",   Job::kPirate,  24, 7,  3,  7,  8, 0,  6,  6, Team::kEnemy, Weapon::kStealAxe },
    { "�K���_��",   Job::kHunter,  18, 6,  1,  5,  5, 0,  3,  6, Team::kEnemy, Weapon::kBow },
    { "�K���_��",   Job::kThief,   16, 3,  1,  2,  9, 0,  2,  7, Team::kEnemy, Weapon::kIronSword },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
    { "�K���_��",   Job::kPirate,  18, 5,  1,  5,  6, 0,  4,  6, Team::kEnemy, Weapon::kAxe },
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
/// ���l��0���߂���������ɐ��`����
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
/// ���j�b�g���������Ă��镐����\��
/// </summary>
/// <param name="weapon"></param>
/// <returns></returns>
std::string DisplayUnitWeaponParameter(Weapon weapon) {
    WeaponDescription weapon_data = Weapon_list_[weapon];
    std::string weapon_parameter;
    weapon_parameter += weapon_data.name;
    weapon_parameter += " (�_���[�W:" + std::to_string(weapon_data.power);
    weapon_parameter += " �d��:" + std::to_string(weapon_data.weight);
    weapon_parameter += " ������:" + std::to_string(weapon_data.hit);
    weapon_parameter += " �K�E��:" + std::to_string(weapon_data.critical);
    weapon_parameter += " �˒�:" + std::to_string(weapon_data.range_min) + "�`" + std::to_string(weapon_data.range_max);
    weapon_parameter += ")\n";

    return weapon_parameter;
}

/// <summary>
/// ���j�b�g���\��
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
    parameter += "�́@�@�@�@�F" + FormatFillDigitWithZero(kDigit, unit_list_[index].strength) + "\n";
    parameter += "�Z�@�@�@�@�F" + FormatFillDigitWithZero(kDigit, unit_list_[index].skill) + "\n";
    parameter += "���탌�x���F" + FormatFillDigitWithZero(kDigit, unit_list_[index].weapon_level) + "\n";
    parameter += "�f�����@�@�F" + FormatFillDigitWithZero(kDigit, unit_list_[index].agility) + "\n";
    parameter += "�^�@�@�@�@�F" + FormatFillDigitWithZero(kDigit, unit_list_[index].luck) + "\n";
    parameter += "�h��́@�@�F" + FormatFillDigitWithZero(kDigit, unit_list_[index].defence) + "\n";
    parameter += "�ړ��́@�@�F" + FormatFillDigitWithZero(kDigit, unit_list_[index].move) + "\n";

    parameter += DisplayUnitWeaponParameter(unit_list_[index].weapon);

    return parameter;
}

/// <summary>
/// �n�`���\��
/// </summary>
/// <param name="position"></param>
/// <returns></returns>
std::string DisplayCellParameter(MapPosition position) {
    std::string parameter;
    CellDescription cell = cell_list_[cells[position.y][position.x]];

    parameter += cell.name + "\n";

    const int kDigit = 2;
    parameter += "�h�����:" + FormatFillDigitWithZero(kDigit, cell.defence) + "%\n";

    parameter += "�񕜌���:";
    parameter += (cell.heal ? "����" : "�Ȃ�");
    parameter += "\n";

    return parameter;
}

/// <summary>
/// �t�F�[�Y�K�C�_���X�\��
/// </summary>
/// <param name="phase"></param>
/// <returns></returns>
std::string DisplayPhaseGuidance(Phase phase) {
    std::string result;
    switch (phase_) {
    case kSelectUnit:       result = "���j�b�g��I�����Ă��������B";  break;
    case kSetMovePosition:  result = "�ړ����ݒ肵�Ă��������B";  break;
    case kSelectAttackUnit: result = "�U���Ώۂ�I��ł��������B";  break;
    default:
        break;
    }
    result += "\n\n";
    return result;
}

// �J�[�\��
MapPosition cursor_position{0, 0};

/// <summary>
/// �ړ��\�͈͂̓h��Ԃ�����X�V
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
/// �J�[�\�����W�ƈ�v���邩�m�F
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>true�F��v�Afalse�F�s��v</returns>
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
/// �J�[�\���ړ�����
/// </summary>
/// <param name="input_param"></param>
void MoveCursor(WPARAM input_param) {
    AtlTrace("KeyUp = %Xh\n", input_param);

    switch (input_param) {
    case 'w': cursor_position.y--; break;
    case 's': cursor_position.y++; break;
    case 'a': cursor_position.x--; break;
    case 'd': cursor_position.x++; break;

    // Enter�L�[������
    case '\r':
    {
        switch (phase_) {
        case Phase::kSelectUnit:
        {
            // �J�[�\���ʒu�Ƀ��j�b�g�����݂��Ȃ��ꍇ�A�X�L�b�v
            int index = GetUnitIndex(cursor_position);
            if (index <= kUndefined) {
                break;
            }
            // �h��Ԃ����菉����
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

    // ��ʒ[�𒴂����ꍇ�A���΂Ƀ��[�v����
    cursor_position.x = (kMapWidth + cursor_position.x) % kMapWidth;
    cursor_position.y = (kMapHeight + cursor_position.y) % kMapHeight;
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
    for (int y = 0; y < kMapHeight; y++) {
        for (int x = 0; x < kMapWidth; x++) {
            if (IsMatchCursorPosition(x, y)) {
                draw_map += "��";
                continue;
            }

            if (fill[y][x]) {
                draw_map += "��";
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
        return 0;

        //----�y�C���g----
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &paintstruct);

        SetBkColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, OPAQUE);
        SetTextColor(hdc, RGB(255, 255, 255));

        // TODO:��悸�`��̈�̏㏑���ɂ��A�����폜
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
    for (int y = 0; y < kMapHeight; y++) {
        for (int x = 0; x < kMapWidth; x++) {
            char c = map_data_[(y * kMapWidth) + x];
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
