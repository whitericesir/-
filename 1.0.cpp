#include <graphics.h>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <cstdlib>

using namespace std;

struct Card
{
    int suit;
    int rank;
    bool up;
};

const int WIDTH = 1400;
const int HEIGHT = 900;
const int CARD_W = 90;
const int CARD_H = 120;
const int COLS = 10;

vector<Card> columns[COLS];
vector<Card> deck;

int selectedCol = -1;
int selectedIndex = -1;

wstring suitText(int s)
{
    switch (s)
    {
    case 0:return L"♠";
    case 1:return L"♥";
    case 2:return L"♣";
    case 3:return L"♦";
    }
    return L"?";
}

wstring rankText(int r)
{
    if (r == 1) return L"A";
    if (r == 11) return L"J";
    if (r == 12) return L"Q";
    if (r == 13) return L"K";

    wchar_t buf[10];
    swprintf(buf, 10, L"%d", r);
    return buf;
}

void createDeck()
{
    deck.clear();

    for (int t = 0; t < 4; t++)
    {
        for (int s = 0; s < 4; s++)
        {
            for (int r = 1; r <= 13; r++)
            {
                deck.push_back({ s,r,false });
            }
        }
    }
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
    {
        columns[i].clear();
    }

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            columns[i].push_back(deck.back());
            deck.pop_back();
        }
    }

    for (int i = 4; i < 10; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            columns[i].push_back(deck.back());
            deck.pop_back();
        }
    }

    for (int i = 0; i < COLS; i++)
    {
        columns[i].back().up = true;
    }
}

void drawCard(int x, int y, Card& c, bool selected)
{
    setfillcolor(selected ? RGB(255, 255, 180) : WHITE);

    solidroundrect(x, y, x + CARD_W, y + CARD_H, 10, 10);

    setlinecolor(BLACK);
    roundrect(x, y, x + CARD_W, y + CARD_H, 10, 10);

    settextcolor((c.suit == 1 || c.suit == 3) ? RED : BLACK);

    settextstyle(25, 0, L"Segoe UI Symbol");

    wstring txt = suitText(c.suit) + rankText(c.rank);

    outtextxy(x + 10, y + 10, txt.c_str());

    settextstyle(45, 0, L"Segoe UI Symbol");

    outtextxy(x + 20, y + 40, suitText(c.suit).c_str());
}

void drawBack(int x, int y)
{
    setfillcolor(BLUE);
    solidroundrect(x, y, x + CARD_W, y + CARD_H, 10, 10);

    setlinecolor(WHITE);
    rectangle(x + 10, y + 10, x + CARD_W - 10, y + CARD_H - 10);
}

void drawGame()
{
    setbkcolor(RGB(0, 120, 0));
    cleardevice();

    settextcolor(WHITE);
    settextstyle(28, 0, L"微软雅黑");

    outtextxy(20, 10, L"蜘蛛纸牌 EasyX 可玩版");

    wchar_t buf[100];
    swprintf(buf, 100, L"剩余牌堆: %d", (int)deck.size());

    outtextxy(300, 10, buf);

    for (int c = 0; c < COLS; c++)
    {
        int x = 50 + c * 125;

        for (int i = 0; i < columns[c].size(); i++)
        {
            int y = 80 + i * 30;

            bool sel = (c == selectedCol && i >= selectedIndex);

            if (columns[c][i].up)
                drawCard(x, y, columns[c][i], sel);
            else
                drawBack(x, y);
        }
    }

    settextstyle(22, 0, L"微软雅黑");

    outtextxy(20, 820,
        L"点击牌组 -> 点击目标列移动 | D发牌 | R重新开始");
}

bool validSequence(int col, int start)
{
    for (int i = start; i < columns[col].size() - 1; i++)
    {
        Card a = columns[col][i];
        Card b = columns[col][i + 1];

        if (!a.up || !b.up)
            return false;

        if (a.suit != b.suit)
            return false;

        if (a.rank != b.rank + 1)
            return false;
    }

    return true;
}

bool canMove(int from, int start, int to)
{
    if (from == to)
        return false;

    if (!validSequence(from, start))
        return false;

    Card moving = columns[from][start];

    if (columns[to].empty())
        return moving.rank == 13;

    Card target = columns[to].back();

    return target.up &&
        target.rank == moving.rank + 1;
}

void moveCards(int from, int start, int to)
{
    vector<Card> temp;

    for (int i = start; i < columns[from].size(); i++)
    {
        temp.push_back(columns[from][i]);
    }

    columns[from].erase(
        columns[from].begin() + start,
        columns[from].end()
    );

    for (auto& c : temp)
    {
        columns[to].push_back(c);
    }

    if (!columns[from].empty())
        columns[from].back().up = true;
}

void checkComplete(int col)
{
    if (columns[col].size() < 13)
        return;

    int start = columns[col].size() - 13;

    int suit = columns[col][start].suit;

    for (int i = 0; i < 13; i++)
    {
        Card c = columns[col][start + i];

        if (!c.up) return;
        if (c.suit != suit) return;
        if (c.rank != 13 - i) return;
    }

    columns[col].erase(
        columns[col].begin() + start,
        columns[col].end()
    );

    MessageBox(GetHWnd(),
        L"完成一组同花顺！",
        L"蜘蛛纸牌",
        MB_OK);
}

void dealCards()
{
    for (int i = 0; i < COLS; i++)
    {
        if (columns[i].empty())
        {
            MessageBox(GetHWnd(),
                L"存在空列，不能发牌！",
                L"错误",
                MB_OK);

            return;
        }
    }

    if (deck.size() < 10)
        return;

    for (int i = 0; i < COLS; i++)
    {
        Card c = deck.back();
        deck.pop_back();

        c.up = true;

        columns[i].push_back(c);
    }
}

bool clickCard(int mx, int my)
{
    for (int c = COLS - 1; c >= 0; c--)
    {
        int x = 50 + c * 125;

        for (int i = columns[c].size() - 1; i >= 0; i--)
        {
            int y = 80 + i * 30;

            if (mx >= x && mx <= x + CARD_W &&
                my >= y && my <= y + CARD_H)
            {
                if (!columns[c][i].up)
                    return true;

                if (!validSequence(c, i))
                    return true;

                selectedCol = c;
                selectedIndex = i;

                return true;
            }
        }
    }

    return false;
}

int clickColumn(int mx)
{
    for (int c = 0; c < COLS; c++)
    {
        int x = 50 + c * 125;

        if (mx >= x && mx <= x + CARD_W)
            return c;
    }

    return -1;
}

int main()
{
    initgraph(WIDTH, HEIGHT);

    BeginBatchDraw();

    initGame();

    while (true)
    {
        drawGame();

        FlushBatchDraw();

        if (MouseHit())
        {
            MOUSEMSG msg = GetMouseMsg();

            if (msg.uMsg == WM_LBUTTONDOWN)
            {
                if (selectedCol == -1)
                {
                    clickCard(msg.x, msg.y);
                }
                else
                {
                    int target = clickColumn(msg.x);

                    if (target != -1)
                    {
                        if (canMove(selectedCol,
                            selectedIndex,
                            target))
                        {
                            moveCards(selectedCol,
                                selectedIndex,
                                target);

                            checkComplete(target);
                        }
                    }

                    selectedCol = -1;
                    selectedIndex = -1;
                }
            }
        }

        if (GetAsyncKeyState('D') & 0x8000)
        {
            dealCards();
            Sleep(200);
        }

        if (GetAsyncKeyState('R') & 0x8000)
        {
            initGame();
            Sleep(200);
        }

        Sleep(16);
    }

    EndBatchDraw();

    closegraph();

    return 0;
}
