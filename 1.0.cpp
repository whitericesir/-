// ======================================================
// 蜘蛛纸牌 豪华终极版（完整修复版）
// VS2022 + EasyX
//
// 功能：
// 1. PNG透明扑克（无黑边）
// 2. 高级柔和阴影
// 3. 半透明拖拽
// 4. 3D翻牌动画
// 5. 真正发牌动画（从牌堆飞入）
// 6. 丝滑移动动画（缓动曲线）
// 7. 发牌按钮 / 撤回按钮 / Ctrl+Z
// 8. 递归自动收牌
// 9. 胜利检测
// ======================================================

#include <graphics.h>
#include <conio.h>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <cwchar>
#include <stack>
#include <windows.h>

using namespace std;

// ======================================================
// 卡牌结构
// ======================================================
struct Card
{
    int suit;   // 0:spades,1:hearts,2:clubs,3:diamonds
    int rank;   // 1=A,11=J,12=Q,13=K
    bool isUp;
};

// ======================================================
// 动画结构
// ======================================================
struct DealAnim
{
    bool active = false;
    Card card;
    int fromX, fromY;
    int toX, toY;
    float progress = 0;
    int targetCol;
};
vector<DealAnim> dealAnims;

struct FlipAnim
{
    bool active = false;
    int col;
    int index;
    float progress = 0;
};
FlipAnim flipAnim;

struct MoveAnimation
{
    bool active = false;
    vector<Card> cards;
    int fromX, fromY;
    int toX, toY;
    float progress = 0;
    int targetCol;
};
MoveAnimation anim;

// ======================================================
// 撤回记录
// ======================================================
struct MoveRecord
{
    vector<Card> cols[10];
    vector<Card> deck;
};

// ======================================================
// 游戏参数
// ======================================================
const int WIDTH = 1600;
const int HEIGHT = 900;
const int CARD_W = 100;
const int CARD_H = 140;
const int COLS = 10;
const int START_X = 40;
const int START_Y = 100;
const int GAP_X = 150;
const float ANIM_SPEED = 0.12f;      // 动画速度

// ======================================================
// 全局变量
// ======================================================
vector<Card> columns[COLS];
vector<Card> deck;
stack<MoveRecord> undoStack;

IMAGE cardImages[4][14];
IMAGE backImage;

bool dragging = false;
vector<Card> dragCards;
int dragFromCol = -1;
int dragStartIndex = -1;
int mouseX = 0, mouseY = 0;
int offsetX = 0, offsetY = 0;

bool isAnimating = false;   // 动画互斥锁

// ======================================================
// 函数声明
// ======================================================
void drawPNG(IMAGE* img, int x, int y, BYTE alpha = 255);
void saveState();
void undo();
void loadCardImages();
void createDeck();
void shuffleDeck();
void initGame();
int calcCardY(int col, int index);
void drawCard(int x, int y, Card& c, bool selected, BYTE alpha = 255);
void drawBack(int x, int y);
void drawBackground();
void drawFlipAnimation();
void drawDealAnimations();
void drawDraggingCards();
void drawAnimation();
void drawGame();
bool validSequence(int col, int start);
bool canMove(int from, int start, int to);
void autoRemove();
bool checkWin();
void dealNewCards();
void moveCards(int from, int start, int to);
bool clickCard(int mx, int my);
int getColumn(int mx);

// ======================================================
// PNG透明绘制（支持全局透明度）
// ======================================================
void drawPNG(IMAGE* img, int x, int y, BYTE alpha)
{
    DWORD* dst = GetImageBuffer();
    DWORD* draw = GetImageBuffer(img);
    int w = img->getwidth();
    int h = img->getheight();
    int dstW = getwidth();
    int dstH = getheight();

    for (int iy = 0; iy < h; iy++)
    {
        for (int ix = 0; ix < w; ix++)
        {
            int dstX = x + ix;
            int dstY = y + iy;
            if (dstX < 0 || dstX >= dstW || dstY < 0 || dstY >= dstH)
                continue;

            DWORD srcColor = draw[iy * w + ix];
            BYTE srcA = (srcColor >> 24) & 0xff;
            srcA = srcA * alpha / 255;
            if (srcA > 0)
                dst[dstY * dstW + dstX] = srcColor;
        }
    }
}

// ======================================================
// 状态保存与撤回
// ======================================================
void saveState()
{
    MoveRecord rec;
    for (int i = 0; i < COLS; i++)
        rec.cols[i] = columns[i];
    rec.deck = deck;
    undoStack.push(rec);
}

void undo()
{
    if (isAnimating || undoStack.empty()) return;

    // 停止所有动画
    anim.active = false;
    flipAnim.active = false;
    dealAnims.clear();
    dragging = false;
    isAnimating = false;

    MoveRecord rec = undoStack.top();
    undoStack.pop();
    for (int i = 0; i < COLS; i++)
        columns[i] = rec.cols[i];
    deck = rec.deck;
}

// ======================================================
// 加载图片（需在工程目录下存在 cards 文件夹及对应PNG）
// ======================================================
void loadCardImages()
{
    const wchar_t* suits[4] = { L"spades", L"hearts", L"clubs", L"diamonds" };
    for (int s = 0; s < 4; s++)
    {
        for (int r = 1; r <= 13; r++)
        {
            wchar_t path[200];
            wchar_t rankStr[10];
            if (r == 1) wcscpy_s(rankStr, L"A");
            else if (r == 11) wcscpy_s(rankStr, L"J");
            else if (r == 12) wcscpy_s(rankStr, L"Q");
            else if (r == 13) wcscpy_s(rankStr, L"K");
            else swprintf_s(rankStr, L"%02d", r);

            swprintf_s(path, L"cards/card_%s_%s.png", suits[s], rankStr);
            loadimage(&cardImages[s][r], path, CARD_W, CARD_H, true);
        }
    }
    loadimage(&backImage, L"cards/card_back.png", CARD_W, CARD_H, true);
}

// ======================================================
// 牌堆初始化与洗牌
// ======================================================
void createDeck()
{
    deck.clear();
    for (int t = 0; t < 4; t++)               // 4套牌
        for (int s = 0; s < 4; s++)
            for (int r = 1; r <= 13; r++)
                deck.push_back({ s, r, false });
}

void shuffleDeck()
{
    srand((unsigned)time(NULL));
    random_shuffle(deck.begin(), deck.end());
}

void initGame()
{
    createDeck();
    shuffleDeck();
    for (int i = 0; i < COLS; i++)
        columns[i].clear();

    // 前4列6张，后6列5张
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 6; j++)
        {
            columns[i].push_back(deck.back());
            deck.pop_back();
        }
    for (int i = 4; i < 10; i++)
        for (int j = 0; j < 5; j++)
        {
            columns[i].push_back(deck.back());
            deck.pop_back();
        }

    // 翻开每列最上面一张
    for (int i = 0; i < COLS; i++)
        columns[i].back().isUp = true;

    // 清空动画状态
    isAnimating = false;
    anim.active = false;
    flipAnim.active = false;
    dealAnims.clear();
    dragging = false;
}

// ======================================================
// 坐标计算（根据堆叠偏移）
// ======================================================
int calcCardY(int col, int index)
{
    int y = START_Y;
    for (int i = 0; i < index; i++)
    {
        if (columns[col][i].isUp)
            y += 38;
        else
            y += 16;
    }
    return y;
}

// ======================================================
// 绘制函数（带柔和阴影）
// ======================================================
void drawCard(int x, int y, Card& c, bool selected, BYTE alpha)
{
    // 阴影层（多层渐变）
    drawPNG(&cardImages[c.suit][c.rank], x + 2, y + 2, 40);
    drawPNG(&cardImages[c.suit][c.rank], x + 4, y + 4, 20);
    drawPNG(&cardImages[c.suit][c.rank], x + 6, y + 6, 10);
    drawPNG(&cardImages[c.suit][c.rank], x, y, alpha);

    if (selected)
    {
        setlinecolor(RGB(255, 215, 0));
        setlinestyle(PS_SOLID, 4);
        rectangle(x - 2, y - 2, x + CARD_W + 2, y + CARD_H + 2);
    }
}

void drawBack(int x, int y)
{
    drawPNG(&backImage, x + 2, y + 2, 40);
    drawPNG(&backImage, x + 4, y + 4, 20);
    drawPNG(&backImage, x + 6, y + 6, 10);
    drawPNG(&backImage, x, y);
}

void drawBackground()
{
    setbkcolor(RGB(20, 120, 40));
    cleardevice();

    setfillcolor(RGB(10, 70, 30));
    solidrectangle(0, 0, WIDTH, 70);

    settextcolor(WHITE);
    settextstyle(34, 0, L"微软雅黑");
    outtextxy(20, 15, L"蜘蛛纸牌 豪华终极版");

    // 发牌按钮
    setfillcolor(RGB(40, 40, 40));
    solidroundrect(WIDTH - 340, 15, WIDTH - 200, 55, 10, 10);
    outtextxy(WIDTH - 300, 22, L"发牌");

    // 撤回按钮
    solidroundrect(WIDTH - 180, 15, WIDTH - 40, 55, 10, 10);
    outtextxy(WIDTH - 140, 22, L"撤回");

    // 显示剩余牌数
    wchar_t txt[100];
    swprintf_s(txt, L"剩余牌数：%d", (int)deck.size());
    settextstyle(24, 0, L"微软雅黑");
    outtextxy(650, 22, txt);
}

// ======================================================
// 翻牌动画（3D缩放效果）
// ======================================================
void drawFlipAnimation()
{
    if (!flipAnim.active) return;

    Card& c = columns[flipAnim.col][flipAnim.index];
    int x = START_X + flipAnim.col * GAP_X;
    int y = calcCardY(flipAnim.col, flipAnim.index);
    float t = flipAnim.progress;

    if (t < 0.5f)
    {
        int w = CARD_W * (1 - t * 2);
        drawBack(x + (CARD_W - w) / 2, y);
    }
    else
    {
        int w = CARD_W * ((t - 0.5f) * 2);
        drawCard(x + (CARD_W - w) / 2, y, c, false);
    }
}

// ======================================================
// 发牌动画（从右下角飞入）
// ======================================================
void drawDealAnimations()
{
    for (auto& d : dealAnims)
    {
        float t = d.progress;
        t = 1 - (1 - t) * (1 - t);   // ease-out
        int x = d.fromX + (d.toX - d.fromX) * t;
        int y = d.fromY + (d.toY - d.fromY) * t;
        drawCard(x, y, d.card, false);
    }
}

// ======================================================
// 拖拽绘制（半透明）
// ======================================================
void drawDraggingCards()
{
    if (!dragging) return;
    int x = mouseX - offsetX;
    int y = mouseY - offsetY;
    for (size_t i = 0; i < dragCards.size(); i++)
        drawCard(x, y + i * 38, dragCards[i], true, 180);
}

// ======================================================
// 移动动画（缓动）
// ======================================================
void drawAnimation()
{
    if (!anim.active) return;
    float t = anim.progress;
    t = 1 - (1 - t) * (1 - t);
    int x = anim.fromX + (anim.toX - anim.fromX) * t;
    int y = anim.fromY + (anim.toY - anim.fromY) * t;
    for (size_t i = 0; i < anim.cards.size(); i++)
        drawCard(x, y + i * 38, anim.cards[i], false);
}

// ======================================================
// 绘制整个游戏界面
// ======================================================
void drawGame()
{
    drawBackground();

    for (int c = 0; c < COLS; c++)
    {
        int x = START_X + c * GAP_X;
        for (size_t i = 0; i < columns[c].size(); i++)
        {
            // 跳过正在拖拽的牌
            bool hidden = (dragging && c == dragFromCol && i >= dragStartIndex);
            // 跳过正在翻牌的牌（由动画绘制）
            bool isFlipping = (flipAnim.active && flipAnim.col == c && flipAnim.index == (int)i);
            if (hidden || isFlipping) continue;

            int y = calcCardY(c, i);
            if (columns[c][i].isUp)
                drawCard(x, y, columns[c][i], false);
            else
                drawBack(x, y);
        }
    }

    drawDraggingCards();
    drawAnimation();
    drawFlipAnimation();   // 放在最上层
    drawDealAnimations();
}

// ======================================================
// 游戏规则判定
// ======================================================
bool validSequence(int col, int start)
{
    for (size_t i = start; i < columns[col].size() - 1; i++)
    {
        Card& a = columns[col][i];
        Card& b = columns[col][i + 1];
        if (!a.isUp || !b.isUp) return false;
        if (a.suit != b.suit) return false;
        if (a.rank != b.rank + 1) return false;
    }
    return true;
}

bool canMove(int from, int start, int to)
{
    if (from == to) return false;
    if (!validSequence(from, start)) return false;
    Card& moving = columns[from][start];
    if (columns[to].empty())
        return (moving.rank == 13);
    Card& target = columns[to].back();
    return (target.rank == moving.rank + 1);
}

// ======================================================
// 自动收牌（递归直到无完整序列）
// ======================================================
void autoRemove()
{
    bool removed = true;
    while (removed)
    {
        removed = false;
        for (int c = 0; c < COLS; c++)
        {
            if (columns[c].size() < 13) continue;
            int start = (int)columns[c].size() - 13;
            int suit = columns[c][start].suit;
            bool ok = true;
            for (int i = 0; i < 13; i++)
            {
                Card& card = columns[c][start + i];
                if (card.suit != suit || card.rank != 13 - i)
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
            {
                columns[c].erase(columns[c].end() - 13, columns[c].end());
                removed = true;
                if (!columns[c].empty())
                    columns[c].back().isUp = true;
                break;   // 重新扫描
            }
        }
    }
}

bool checkWin()
{
    for (int i = 0; i < COLS; i++)
        if (!columns[i].empty()) return false;
    return true;
}

// ======================================================
// 发牌（动画）
// ======================================================
void dealNewCards()
{
    if (isAnimating) return;

    for (int i = 0; i < COLS; i++)
        if (columns[i].empty())
        {
            MessageBox(GetHWnd(), L"有空列不能发牌！", L"提示", MB_OK);
            return;
        }
    if (deck.size() < 10)
    {
        MessageBox(GetHWnd(), L"牌堆为空！", L"提示", MB_OK);
        return;
    }

    saveState();
    isAnimating = true;
    dealAnims.clear();

    for (int i = 0; i < COLS; i++)
    {
        Card c = deck.back();
        deck.pop_back();
        c.isUp = true;
        DealAnim d;
        d.card = c;
        d.fromX = WIDTH - 120;
        d.fromY = HEIGHT - 180;
        d.toX = START_X + i * GAP_X;
        d.toY = calcCardY(i, (int)columns[i].size());
        d.progress = 0;
        d.targetCol = i;
        dealAnims.push_back(d);
    }
}

// ======================================================
// 移动牌（带动画）
// ======================================================
void moveCards(int from, int start, int to)
{
    if (isAnimating) return;
    saveState();
    isAnimating = true;

    vector<Card> temp;
    for (size_t i = start; i < columns[from].size(); i++)
        temp.push_back(columns[from][i]);

    anim.fromX = START_X + from * GAP_X;
    anim.fromY = calcCardY(from, start);
    anim.toX = START_X + to * GAP_X;
    anim.toY = columns[to].empty() ? START_Y : calcCardY(to, (int)columns[to].size());
    anim.cards = temp;
    anim.progress = 0;
    anim.active = true;
    anim.targetCol = to;

    // 移除源牌
    columns[from].erase(columns[from].begin() + start, columns[from].end());
    // 翻开新牌顶（如果有且未翻开）
    if (!columns[from].empty() && !columns[from].back().isUp)
    {
        flipAnim.active = true;
        flipAnim.col = from;
        flipAnim.index = (int)columns[from].size() - 1;
        flipAnim.progress = 0;
        columns[from].back().isUp = true;
    }
}

// ======================================================
// 鼠标点击检测（开始拖拽）
// ======================================================
bool clickCard(int mx, int my)
{
    for (int c = COLS - 1; c >= 0; c--)
    {
        int x = START_X + c * GAP_X;
        for (int i = (int)columns[c].size() - 1; i >= 0; i--)
        {
            int y = calcCardY(c, i);
            if (mx >= x && mx <= x + CARD_W && my >= y && my <= y + CARD_H)
            {
                if (!columns[c][i].isUp)   // 点击背面牌：直接翻开（无动画，简单处理）
                {
                    columns[c][i].isUp = true;
                    return true;
                }
                if (!validSequence(c, i)) return true;  // 不能拖拽多张

                // 开始拖拽
                dragging = true;
                dragFromCol = c;
                dragStartIndex = i;
                dragCards.clear();
                for (size_t k = i; k < columns[c].size(); k++)
                    dragCards.push_back(columns[c][k]);
                offsetX = mx - x;
                offsetY = my - y;
                return true;
            }
        }
    }
    return false;
}

int getColumn(int mx)
{
    for (int c = 0; c < COLS; c++)
    {
        int x = START_X + c * GAP_X;
        if (mx >= x && mx <= x + CARD_W)
            return c;
    }
    return -1;
}

// ======================================================
// 主函数
// ======================================================
int main()
{
    initgraph(WIDTH, HEIGHT);
    BeginBatchDraw();
    loadCardImages();
    initGame();

    while (true)
    {
        drawGame();
        if (checkWin())
        {
            if (MessageBox(GetHWnd(), L"恭喜通关！\n是否继续游戏？", L"胜利", MB_YESNO) == IDYES)
                initGame();
            else
                break;
        }
        FlushBatchDraw();

        // Ctrl+Z 撤回（仅当无动画时）
        if ((GetAsyncKeyState('Z') & 0x8000) && (GetAsyncKeyState(VK_CONTROL) & 0x8000) && !isAnimating)
        {
            undo();
            Sleep(200);
        }

        // 更新移动动画
        if (anim.active)
        {
            anim.progress += ANIM_SPEED;
            if (anim.progress >= 1.0f)
            {
                for (auto& c : anim.cards)
                    columns[anim.targetCol].push_back(c);
                autoRemove();
                anim.active = false;
                if (!flipAnim.active && dealAnims.empty())
                    isAnimating = false;
            }
        }

        // 更新翻牌动画
        if (flipAnim.active)
        {
            flipAnim.progress += ANIM_SPEED;
            if (flipAnim.progress >= 1.0f)
            {
                flipAnim.active = false;
                if (!anim.active && dealAnims.empty())
                    isAnimating = false;
            }
        }

        // 更新发牌动画
        for (int i = 0; i < (int)dealAnims.size(); i++)
        {
            dealAnims[i].progress += ANIM_SPEED;
            if (dealAnims[i].progress >= 1.0f)
            {
                columns[dealAnims[i].targetCol].push_back(dealAnims[i].card);
                dealAnims.erase(dealAnims.begin() + i);
                i--;
            }
        }
        if (dealAnims.empty() && !anim.active && !flipAnim.active)
        {
            isAnimating = false;
            autoRemove();   // 发牌结束后检查收牌
        }

        // 鼠标事件（动画期间禁止操作）
        if (!isAnimating)
        {
            while (MouseHit())
            {
                MOUSEMSG msg = GetMouseMsg();
                switch (msg.uMsg)
                {
                case WM_LBUTTONDOWN:
                    // 按钮检测
                    if (msg.x >= WIDTH - 340 && msg.x <= WIDTH - 200 && msg.y >= 15 && msg.y <= 55)
                        dealNewCards();
                    else if (msg.x >= WIDTH - 180 && msg.x <= WIDTH - 40 && msg.y >= 15 && msg.y <= 55)
                        undo();
                    else
                        clickCard(msg.x, msg.y);
                    break;

                case WM_MOUSEMOVE:
                    mouseX = msg.x;
                    mouseY = msg.y;
                    break;

                case WM_LBUTTONUP:
                    if (dragging)
                    {
                        int target = getColumn(msg.x);
                        if (target != -1 && canMove(dragFromCol, dragStartIndex, target))
                            moveCards(dragFromCol, dragStartIndex, target);
                        dragging = false;
                        dragCards.clear();
                        dragFromCol = -1;
                        dragStartIndex = -1;
                    }
                    break;
                }
            }
        }
        else
        {
            // 动画期间清空鼠标消息，避免堆积
            while (MouseHit()) GetMouseMsg();
        }

        Sleep(16);
    }

    EndBatchDraw();
    closegraph();
    return 0;
}