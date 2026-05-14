// ======================================================
// 蜘蛛纸牌 豪华终极完整版
// VS2022 + EasyX
//
// 功能：
// 1. PNG透明扑克（无黑边）
// 2. 半透明拖拽
// 3. 翻牌动画
// 4. 丝滑移动动画
// 5. 发牌按钮
// 6. 撤回按钮
// 7. Ctrl+Z撤回
// 8. 自动收完整序列
// 9. 胜利检测
// 10. 正版蜘蛛纸牌布局
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
// 卡牌
// ======================================================

struct Card
{
    int suit;
    int rank;
    bool isUp;
};

// ======================================================
// 翻牌动画
// ======================================================

struct FlipAnim
{
    bool active = false;

    int col;
    int index;

    float progress = 0;
};

FlipAnim flipAnim;

// ======================================================
// 移动动画
// ======================================================

struct MoveAnimation
{
    bool active = false;

    vector<Card> cards;

    int fromX;
    int fromY;

    int toX;
    int toY;

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
// 参数
// ======================================================

const int WIDTH = 1600;
const int HEIGHT = 900;

const int CARD_W = 100;
const int CARD_H = 140;

const int COLS = 10;

const int START_X = 40;
const int START_Y = 100;

const int GAP_X = 150;

// ======================================================
// 全局变量
// ======================================================

vector<Card> columns[COLS];

vector<Card> deck;

stack<MoveRecord> undoStack;

// 图片
IMAGE cardImages[4][14];

IMAGE backImage;

// 拖拽
bool dragging = false;

vector<Card> dragCards;

int dragFromCol = -1;

int dragStartIndex = -1;

int mouseX = 0;

int mouseY = 0;

int offsetX = 0;

int offsetY = 0;

// ======================================================
// PNG透明绘制
// ======================================================

void drawPNG(
    IMAGE* img,
    int x,
    int y,
    BYTE alpha = 255
)
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

            if (dstX < 0 || dstX >= dstW ||
                dstY < 0 || dstY >= dstH)
            {
                continue;
            }

            DWORD srcColor =
                draw[iy * w + ix];

            BYTE srcA =
                (srcColor >> 24) & 0xff;

            srcA = srcA * alpha / 255;

            if (srcA > 0)
            {
                dst[dstY * dstW + dstX] =
                    srcColor;
            }
        }
    }
}

// ======================================================
// 保存状态
// ======================================================

void saveState()
{
    MoveRecord rec;

    for (int i = 0; i < COLS; i++)
    {
        rec.cols[i] = columns[i];
    }

    rec.deck = deck;

    undoStack.push(rec);
}

// ======================================================
// 撤回
// ======================================================

void undo()
{
    if (undoStack.empty())
        return;

    MoveRecord rec =
        undoStack.top();

    undoStack.pop();

    for (int i = 0; i < COLS; i++)
    {
        columns[i] = rec.cols[i];
    }

    deck = rec.deck;
}

// ======================================================
// 加载图片
// ======================================================

void loadCardImages()
{
    const wchar_t* suits[4] =
    {
        L"spades",
        L"hearts",
        L"clubs",
        L"diamonds"
    };

    for (int s = 0; s < 4; s++)
    {
        for (int r = 1; r <= 13; r++)
        {
            wchar_t path[200];

            wchar_t rankStr[10];

            if (r == 1)
                wcscpy_s(rankStr, L"A");
            else if (r == 11)
                wcscpy_s(rankStr, L"J");
            else if (r == 12)
                wcscpy_s(rankStr, L"Q");
            else if (r == 13)
                wcscpy_s(rankStr, L"K");
            else
                swprintf_s(rankStr, L"%02d", r);

            swprintf_s(
                path,
                L"cards/card_%s_%s.png",
                suits[s],
                rankStr
            );

            loadimage(
                &cardImages[s][r],
                path,
                CARD_W,
                CARD_H,
                true
            );
        }
    }

    loadimage(
        &backImage,
        L"cards/card_back.png",
        CARD_W,
        CARD_H,
        true
    );
}

// ======================================================
// 创建牌堆
// ======================================================

void createDeck()
{
    deck.clear();

    for (int t = 0; t < 4; t++)
    {
        for (int s = 0; s < 4; s++)
        {
            for (int r = 1; r <= 13; r++)
            {
                deck.push_back(
                    { s,r,false }
                );
            }
        }
    }
}

// ======================================================
// 洗牌
// ======================================================

void shuffleDeck()
{
    srand((unsigned)time(NULL));

    random_shuffle(
        deck.begin(),
        deck.end()
    );
}

// ======================================================
// 初始化
// ======================================================

void initGame()
{
    createDeck();

    shuffleDeck();

    for (int i = 0; i < COLS; i++)
    {
        columns[i].clear();
    }

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            columns[i].push_back(
                deck.back()
            );

            deck.pop_back();
        }
    }

    for (int i = 4; i < 10; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            columns[i].push_back(
                deck.back()
            );

            deck.pop_back();
        }
    }

    for (int i = 0; i < COLS; i++)
    {
        columns[i].back().isUp = true;
    }
}

// ======================================================
// 计算Y
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
// 绘制扑克
// ======================================================

void drawCard(
    int x,
    int y,
    Card& c,
    bool selected,
    BYTE alpha = 255
)
{
    drawPNG(
        &cardImages[c.suit][c.rank],
        x,
        y,
        alpha
    );

    if (selected)
    {
        setlinecolor(YELLOW);

        setlinestyle(
            PS_SOLID,
            4
        );

        rectangle(
            x - 2,
            y - 2,
            x + CARD_W + 2,
            y + CARD_H + 2
        );
    }
}

// ======================================================
// 绘制背面
// ======================================================

void drawBack(
    int x,
    int y
)
{
    drawPNG(
        &backImage,
        x,
        y
    );
}

// ======================================================
// 背景
// ======================================================

void drawBackground()
{
    setbkcolor(
        RGB(20, 120, 40)
    );

    cleardevice();

    // 顶栏
    setfillcolor(
        RGB(10, 70, 30)
    );

    solidrectangle(
        0,
        0,
        WIDTH,
        70
    );

    settextcolor(WHITE);

    settextstyle(
        34,
        0,
        L"微软雅黑"
    );

    outtextxy(
        20,
        15,
        L"蜘蛛纸牌 豪华终极版"
    );

    // 发牌按钮
    setfillcolor(
        RGB(40, 40, 40)
    );

    solidroundrect(
        WIDTH - 340,
        15,
        WIDTH - 200,
        55,
        10,
        10
    );

    outtextxy(
        WIDTH - 300,
        22,
        L"发牌"
    );

    // 撤回按钮
    solidroundrect(
        WIDTH - 180,
        15,
        WIDTH - 40,
        55,
        10,
        10
    );

    outtextxy(
        WIDTH - 140,
        22,
        L"撤回"
    );

    // 剩余牌数
    wchar_t txt[100];

    swprintf_s(
        txt,
        L"剩余牌数：%d",
        (int)deck.size()
    );

    outtextxy(
        650,
        18,
        txt
    );
}

// ======================================================
// 翻牌动画
// ======================================================

void drawFlipAnimation()
{
    if (!flipAnim.active)
        return;

    Card& c =
        columns[flipAnim.col]
        [flipAnim.index];

    int x =
        START_X +
        flipAnim.col * GAP_X;

    int y =
        calcCardY(
            flipAnim.col,
            flipAnim.index
        );

    float t =
        flipAnim.progress;

    int w;

    if (t < 0.5f)
    {
        w =
            CARD_W *
            (1 - t * 2);

        drawBack(
            x + (CARD_W - w) / 2,
            y
        );
    }
    else
    {
        w =
            CARD_W *
            ((t - 0.5f) * 2);

        drawCard(
            x + (CARD_W - w) / 2,
            y,
            c,
            false
        );
    }
}

// ======================================================
// 半透明拖拽
// ======================================================

void drawDraggingCards()
{
    if (!dragging)
        return;

    int x =
        mouseX - offsetX;

    int y =
        mouseY - offsetY;

    for (int i = 0; i < dragCards.size(); i++)
    {
        drawCard(
            x,
            y + i * 38,
            dragCards[i],
            true,
            180
        );
    }
}

// ======================================================
// 移动动画
// ======================================================

void drawAnimation()
{
    if (!anim.active)
        return;

    float t =
        anim.progress;

    t =
        1 - (1 - t) * (1 - t);

    int x =
        anim.fromX +
        (anim.toX - anim.fromX) * t;

    int y =
        anim.fromY +
        (anim.toY - anim.fromY) * t;

    for (int i = 0; i < anim.cards.size(); i++)
    {
        drawCard(
            x,
            y + i * 38,
            anim.cards[i],
            false
        );
    }
}

// ======================================================
// 绘制游戏
// ======================================================

void drawGame()
{
    drawBackground();

    for (int c = 0; c < COLS; c++)
    {
        int x =
            START_X + c * GAP_X;

        for (int i = 0; i < columns[c].size(); i++)
        {
            bool hidden = false;

            if (dragging &&
                c == dragFromCol &&
                i >= dragStartIndex)
            {
                hidden = true;
            }

            if (hidden)
                continue;

            int y =
                calcCardY(c, i);

            if (columns[c][i].isUp)
            {
                drawCard(
                    x,
                    y,
                    columns[c][i],
                    false
                );
            }
            else
            {
                drawBack(
                    x,
                    y
                );
            }
        }
    }

    drawDraggingCards();

    drawAnimation();

    drawFlipAnimation();
}

// ======================================================
// 连续检测
// ======================================================

bool validSequence(
    int col,
    int start
)
{
    for (
        int i = start;
        i < columns[col].size() - 1;
        i++
        )
    {
        Card a =
            columns[col][i];

        Card b =
            columns[col][i + 1];

        if (!a.isUp ||
            !b.isUp)
            return false;

        if (a.suit != b.suit)
            return false;

        if (a.rank != b.rank + 1)
            return false;
    }

    return true;
}

// ======================================================
// 是否能移动
// ======================================================

bool canMove(
    int from,
    int start,
    int to
)
{
    if (from == to)
        return false;

    if (!validSequence(
        from,
        start
    ))
        return false;

    Card moving =
        columns[from][start];

    if (columns[to].empty())
    {
        return moving.rank == 13;
    }

    Card target =
        columns[to].back();

    return target.rank ==
        moving.rank + 1;
}

// ======================================================
// 自动收牌
// ======================================================

void autoRemove()
{
    for (int c = 0; c < COLS; c++)
    {
        if (columns[c].size() < 13)
            continue;

        bool ok = true;

        int start =
            columns[c].size() - 13;

        int suit =
            columns[c][start].suit;

        for (int i = 0; i < 13; i++)
        {
            Card card =
                columns[c][start + i];

            if (card.suit != suit)
            {
                ok = false;
                break;
            }

            if (card.rank != 13 - i)
            {
                ok = false;
                break;
            }
        }

        if (ok)
        {
            columns[c].erase(
                columns[c].end() - 13,
                columns[c].end()
            );

            if (!columns[c].empty())
            {
                columns[c]
                    .back()
                    .isUp = true;
            }
        }
    }
}

// ======================================================
// 胜利检测
// ======================================================

bool checkWin()
{
    for (int i = 0; i < COLS; i++)
    {
        if (!columns[i].empty())
            return false;
    }

    return true;
}

// ======================================================
// 发牌
// ======================================================

void dealNewCards()
{
    for (int i = 0; i < COLS; i++)
    {
        if (columns[i].empty())
        {
            MessageBox(
                GetHWnd(),
                L"有空列不能发牌！",
                L"提示",
                MB_OK
            );

            return;
        }
    }

    if (deck.size() < 10)
    {
        MessageBox(
            GetHWnd(),
            L"牌堆为空！",
            L"提示",
            MB_OK
        );

        return;
    }

    saveState();

    for (int i = 0; i < COLS; i++)
    {
        Card c =
            deck.back();

        deck.pop_back();

        c.isUp = true;

        columns[i].push_back(c);
    }
}

// ======================================================
// 移动牌
// ======================================================

void moveCards(
    int from,
    int start,
    int to
)
{
    saveState();

    vector<Card> temp;

    for (
        int i = start;
        i < columns[from].size();
        i++
        )
    {
        temp.push_back(
            columns[from][i]
        );
    }

    anim.fromX =
        START_X + from * GAP_X;

    anim.fromY =
        calcCardY(from, start);

    anim.toX =
        START_X + to * GAP_X;

    if (columns[to].empty())
    {
        anim.toY =
            START_Y;
    }
    else
    {
        anim.toY =
            calcCardY(
                to,
                columns[to].size()
            );
    }

    anim.cards =
        temp;

    anim.progress =
        0;

    anim.active =
        true;

    anim.targetCol =
        to;

    columns[from].erase(
        columns[from].begin() + start,
        columns[from].end()
    );

    // 翻牌动画
    if (!columns[from].empty())
    {
        if (!columns[from].back().isUp)
        {
            flipAnim.active = true;

            flipAnim.col = from;

            flipAnim.index =
                columns[from].size() - 1;

            flipAnim.progress = 0;
        }

        columns[from]
            .back()
            .isUp = true;
    }
}

// ======================================================
// 点击牌
// ======================================================

bool clickCard(
    int mx,
    int my
)
{
    for (int c = COLS - 1; c >= 0; c--)
    {
        int x =
            START_X + c * GAP_X;

        for (
            int i = (int)columns[c].size() - 1;
            i >= 0;
            i--
            )
        {
            int y =
                calcCardY(c, i);

            if (mx >= x &&
                mx <= x + CARD_W &&
                my >= y &&
                my <= y + CARD_H)
            {
                if (!columns[c][i].isUp)
                    return true;

                if (!validSequence(c, i))
                    return true;

                dragging = true;

                dragFromCol = c;

                dragStartIndex = i;

                dragCards.clear();

                for (
                    int k = i;
                    k < columns[c].size();
                    k++
                    )
                {
                    dragCards.push_back(
                        columns[c][k]
                    );
                }

                offsetX = mx - x;

                offsetY = my - y;

                return true;
            }
        }
    }

    return false;
}

// ======================================================
// 获取列
// ======================================================

int getColumn(int mx)
{
    for (int c = 0; c < COLS; c++)
    {
        int x =
            START_X + c * GAP_X;

        if (mx >= x &&
            mx <= x + CARD_W)
        {
            return c;
        }
    }

    return -1;
}

// ======================================================
// 主函数
// ======================================================

int main()
{
    initgraph(
        WIDTH,
        HEIGHT
    );

    BeginBatchDraw();

    loadCardImages();

    initGame();

    while (true)
    {
        drawGame();

        // 胜利
        if (checkWin())
        {
            MessageBox(
                GetHWnd(),
                L"恭喜通关！",
                L"胜利",
                MB_OK
            );

            initGame();
        }

        FlushBatchDraw();

        // Ctrl+Z
        if (GetAsyncKeyState('Z') & 0x8000)
        {
            if (GetAsyncKeyState(VK_CONTROL)
                & 0x8000)
            {
                undo();

                Sleep(200);
            }
        }

        // 更新移动动画
        if (anim.active)
        {
            anim.progress += 0.08f;

            if (anim.progress >= 1)
            {
                anim.progress = 1;

                for (auto& c : anim.cards)
                {
                    columns
                        [anim.targetCol]
                        .push_back(c);
                }

                autoRemove();

                anim.active = false;
            }
        }

        // 更新翻牌动画
        if (flipAnim.active)
        {
            flipAnim.progress += 0.08f;

            if (flipAnim.progress >= 1)
            {
                flipAnim.active = false;
            }
        }

        while (MouseHit())
        {
            MOUSEMSG msg =
                GetMouseMsg();

            switch (msg.uMsg)
            {
            case WM_LBUTTONDOWN:
            {
                // 发牌按钮
                if (msg.x >= WIDTH - 340 &&
                    msg.x <= WIDTH - 200 &&
                    msg.y >= 15 &&
                    msg.y <= 55)
                {
                    dealNewCards();

                    break;
                }

                // 撤回按钮
                if (msg.x >= WIDTH - 180 &&
                    msg.x <= WIDTH - 40 &&
                    msg.y >= 15 &&
                    msg.y <= 55)
                {
                    undo();

                    break;
                }

                clickCard(
                    msg.x,
                    msg.y
                );

                break;
            }

            case WM_MOUSEMOVE:
            {
                mouseX = msg.x;

                mouseY = msg.y;

                break;
            }

            case WM_LBUTTONUP:
            {
                if (dragging)
                {
                    int target =
                        getColumn(msg.x);

                    if (target != -1)
                    {
                        if (canMove(
                            dragFromCol,
                            dragStartIndex,
                            target
                        ))
                        {
                            moveCards(
                                dragFromCol,
                                dragStartIndex,
                                target
                            );
                        }
                    }

                    dragging = false;

                    dragCards.clear();

                    dragFromCol = -1;

                    dragStartIndex = -1;
                }

                break;
            }
            }
        }

        Sleep(16);
    }

    EndBatchDraw();

    closegraph();

    return 0;
}