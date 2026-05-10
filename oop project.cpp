#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <windows.h>

using namespace std;
using namespace sf;


// console color helpers
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}
void resetColor() { setColor(7); }

void printBanner() {
    setColor(11);
    cout << "\n";
    cout << "                                 ╔══════════════════════════════════════════════════════╗\n";
    cout << "                                 ║                                                      ║\n";
    setColor(14);
    cout << "                                 ║         ♫  M O O D   M U S I C   P L A Y E R  ♫      ║\n";
    setColor(11);
    cout << "                                 ║                                                      ║\n";
    cout << "                                 ╚══════════════════════════════════════════════════════╝\n";
    resetColor();
    cout << "\n";
}

void printDivider() {
    setColor(8);
    cout << "                                 ────────────────────────────────────────────────────────\n";
    resetColor();
}

void printMenu() {
    printDivider();
    setColor(15);
    cout << "                                                   \xe2\x99\xab  SELECT YOUR MOOD\n";
    printDivider();
    setColor(10);
    cout << "                                                     [1]  \xf0\x9f\x98\x8a  Happy\n";
    setColor(9);
    cout << "                                                     [2]  \xf0\x9f\x98\xa2  Sad\n";
    setColor(13);
    cout << "                                                     [3]  \xe2\x9a\xa1  Energetic\n";
    setColor(14);
    cout << "                                                     [4]  \xf0\x9f\x8e\xb6  Show All Songs\n";
    setColor(11);
    cout << "                                                     [5]  +  Add New Song\n";
    setColor(4);
    cout << "                                                     [6]  X  Exit\n";
    printDivider();
    setColor(15);
    cout << "                                                     >  Enter your choice: ";
    resetColor();
}

void printSongList(const vector<pair<string, string>>& songs) {
    printDivider();
    setColor(14);
    cout << "                                                   \xe2\x99\xab  SONGS IN PLAYLIST:\n";
    printDivider();
    int i = 1;
    for (auto& s : songs) {
        setColor(11);
        cout << "                                              " << i++ << ".  ";
        setColor(15);
        cout << s.first;
        setColor(8);
        cout << "  --  ";
        setColor(10);
        cout << s.second << "\n";
    }
    resetColor();
    printDivider();
}

void printSubMenu() {
    printDivider();
    setColor(15);
    cout << "                                               >  What do you want to do?\n\n";
    setColor(10);
    cout << "                                                   [1]  >   Play\n";
    setColor(14);
    cout << "                                                   [2]  *   View Songs\n";
    setColor(8);
    cout << "                                                   [3]  <   Back\n";
    printDivider();
    setColor(15);
    cout << "                                                   >  Enter choice: ";
    resetColor();
}

void printSuccess(const string& msg) {
    setColor(10);
    cout << "\n                                                   OK  " << msg << "\n";
    resetColor();
}

void printError(const string& msg) {
    setColor(4);
    cout << "\n                                                   X  " << msg << "\n";
    resetColor();
}

// song class 
class Song {
public:
    string title;
    string artist;
    string mood;
    string filePath;

    Song(string t, string a, string m, string p)
        : title(t), artist(a), mood(m), filePath(p) {
    }
};

// button helper struct

struct Button {
    RectangleShape shape;
    optional<Text> label;
    Color normalColor;
    Color hoverColor;
    bool isHovered = false;

    void draw(RenderWindow& win) {
        win.draw(shape);
        if (label) win.draw(*label);
    }

    bool contains(Vector2i pos) {
        return shape.getGlobalBounds().contains(Vector2f((float)pos.x, (float)pos.y));
    }

    void updateHover(Vector2i mousePos) {
        isHovered = contains(mousePos);
        shape.setFillColor(isHovered ? hoverColor : normalColor);
    }
};

// music player class + engine
class MusicPlayer {
private:
    RenderWindow window;
    Font font;
    Music music;
    vector<Song> currentPlaylist;
    int currentIndex = 0;
    bool isPlaying = false;

    CircleShape disc;
    CircleShape discCenter;
    float discAngle = 0.f;

    RectangleShape progressBarBg;
    RectangleShape progressBarFill;
    CircleShape progressDot;

    Button btnPlay;
    Button btnPrev;
    Button btnNext;
    Button btnBack;

    RectangleShape topPanel;
    RectangleShape bottomPanel;

    const float W = 1280.f;
    const float H = 800.f;

    // music player color palette
    Color darkBg = Color(18, 18, 18);   // very dark gray bg
    Color panelCol = Color(28, 28, 28);   // slightly lighter panel
    Color accent = Color(30, 215, 96);   // Spotify green  ← primary
    Color accent2 = Color(0, 255, 255);   // soft cyan glow ← secondary
    Color btnNormal = Color(40, 40, 40);   // dark gray button
    Color btnHover = Color(60, 60, 60);   // hover lift
    Color textCol = Color(255, 255, 255);   // pure white text
    Color subText = Color(179, 179, 179);   // muted gray subtext

    void makeButton(Button& btn, Vector2f pos, Vector2f size,
        const string& labelStr, unsigned int fontSize) {
        btn.shape.setSize(size);
        btn.shape.setPosition(pos);
        btn.shape.setFillColor(btnNormal);
        btn.normalColor = btnNormal;
        btn.hoverColor = btnHover;
        btn.shape.setOutlineThickness(1.f);
        btn.shape.setOutlineColor(Color(70, 70, 70));
        btn.label = Text(font, labelStr, fontSize);
        btn.label->setFillColor(textCol);
        FloatRect lb = btn.label->getGlobalBounds();
        btn.label->setPosition({
            pos.x + (size.x - lb.size.x) / 2.f - lb.position.x,
            pos.y + (size.y - lb.size.y) / 2.f - lb.position.y
            });
    }

public:
    MusicPlayer() {
        if (!font.openFromFile("arial.ttf"))
            cout << "Error: arial.ttf not found!\n";
        setupUI();
    }

    void setupUI() {
        float cx = W / 2.f;
        float topH = H * 0.70f;
        float botH = H - topH;
        float pbX = 160.f;
        float pbW = W - pbX * 2.f;
        float pbY = topH + 28.f;

        topPanel.setSize({ W, topH });
        topPanel.setPosition({ 0.f, 0.f });
        topPanel.setFillColor(panelCol);

        bottomPanel.setSize({ W, botH });
        bottomPanel.setPosition({ 0.f, topH });
        bottomPanel.setFillColor(Color(12, 12, 12));

        disc.setRadius(155.f);
        disc.setOrigin({ 155.f, 155.f });
        disc.setPosition({ cx, 240.f });
        disc.setFillColor(Color(30, 30, 30));
        disc.setOutlineThickness(5.f);
        disc.setOutlineColor(accent);

        discCenter.setRadius(18.f);
        discCenter.setOrigin({ 18.f, 18.f });
        discCenter.setPosition({ cx, 240.f });
        discCenter.setFillColor(accent);

        progressBarBg.setSize({ pbW, 6.f });
        progressBarBg.setPosition({ pbX, pbY });
        progressBarBg.setFillColor(Color(50, 50, 50));

        progressBarFill.setSize({ 0.f, 6.f });
        progressBarFill.setPosition({ pbX, pbY });
        progressBarFill.setFillColor(accent);

        progressDot.setRadius(9.f);
        progressDot.setOrigin({ 9.f, 9.f });
        progressDot.setPosition({ pbX, pbY + 3.f });
        progressDot.setFillColor(accent);

        // 4 buttons evenly spaced across the bottom panel
        // Layout: [<< PREV]  [> PLAY / || PAUSE]  [NEXT >>]  [X BACK]
        float btnY = topH + 75.f;
        float btnH = 54.f;
        float playH = 66.f;

        // Total row width = 4 buttons + 3 gaps
        // Button widths: 160, 180, 160, 160  gaps: 20 each
        float totalW = 160.f + 20.f + 180.f + 20.f + 160.f + 20.f + 160.f; // = 880
        float startX = (W - totalW) / 2.f;  // centered

        makeButton(btnPrev, { startX,                          btnY + 6.f }, { 160.f, btnH }, "<< PREV", 14);
        makeButton(btnPlay, { startX + 160.f + 20.f,           btnY }, { 180.f, playH }, "> PLAY", 15);
        makeButton(btnNext, { startX + 160.f + 20.f + 180.f + 20.f, btnY + 6.f }, { 160.f, btnH }, "NEXT >>", 14);
        makeButton(btnBack, { startX + 160.f + 20.f + 180.f + 20.f + 160.f + 20.f, btnY + 6.f }, { 160.f, btnH }, "X  BACK", 14);

        btnPlay.shape.setOutlineColor(accent);        // green outline on play
        btnPlay.hoverColor = Color(24, 168, 74);       // darker spotify green on hover
        btnBack.shape.setOutlineColor(Color(220, 80, 80));
        btnBack.normalColor = Color(50, 20, 20);
        btnBack.shape.setFillColor(Color(50, 20, 20));
        btnBack.hoverColor = Color(100, 30, 30);
        btnBack.label->setFillColor(Color(255, 100, 100));
    }

    void loadPlaylist(const vector<Song>& songs, int startIndex = 0) {
        if (songs.empty()) return;
        currentPlaylist = songs;
        currentIndex = startIndex;
        playCurrent();
    }

    void playCurrent() {
        if (currentPlaylist.empty()) return;
        if (music.openFromFile(currentPlaylist[currentIndex].filePath)) {
            music.play();
            isPlaying = true;
            updatePlayButton();
        }
        else {
            cout << "File error: " << currentPlaylist[currentIndex].filePath << endl;
        }
    }

    void updatePlayButton() {
        string lbl = isPlaying ? "||  PAUSE" : ">  PLAY";
        btnPlay.label->setString(lbl);
        FloatRect lb = btnPlay.label->getGlobalBounds();
        Vector2f  pos = btnPlay.shape.getPosition();
        Vector2f  sz = btnPlay.shape.getSize();
        btnPlay.label->setPosition({
            pos.x + (sz.x - lb.size.x) / 2.f - lb.position.x,
            pos.y + (sz.y - lb.size.y) / 2.f - lb.position.y
            });
    }

    void run() {
        window.create(VideoMode({ (unsigned int)W, (unsigned int)H }),
            "Mood Music Player", Style::Titlebar | Style::Close);
        window.setFramerateLimit(60);

        VideoMode desktop = VideoMode::getDesktopMode();
        window.setPosition(Vector2i(
            (int)(desktop.size.x / 2 - W / 2),
            (int)(desktop.size.y / 2 - H / 2)
        ));

        while (window.isOpen()) {
            Vector2i mousePos = Mouse::getPosition(window);
            btnPlay.updateHover(mousePos);
            btnPrev.updateHover(mousePos);
            btnNext.updateHover(mousePos);
            btnBack.updateHover(mousePos);

            while (const optional event = window.pollEvent()) {
                if (event->is<Event::Closed>()) {
                    music.stop();
                    window.close();
                }
                if (const auto* m = event->getIf<Event::MouseButtonPressed>())
                    if (m->button == Mouse::Button::Left)
                        handleClicks(m->position);
            }

            if (!currentPlaylist.empty() &&
                music.getStatus() == SoundSource::Status::Playing)
            {
                float pbX = 160.f;
                float pbW = W - pbX * 2.f;
                float pbY = H * 0.70f + 28.f;
                float dur = music.getDuration().asSeconds();
                float prog = (dur > 0) ? music.getPlayingOffset().asSeconds() / dur : 0.f;
                progressBarFill.setSize({ prog * pbW, 6.f });
                progressDot.setPosition({ pbX + prog * pbW, pbY + 3.f });
                discAngle += 0.5f;
                disc.setRotation(degrees(discAngle));
            }

            render();
        }
    }

    void handleClicks(Vector2i pos) {
        if (btnBack.contains(pos)) {
            music.stop();
            window.close();
            return;
        }
        if (btnPlay.contains(pos)) {
            if (isPlaying) { music.pause(); isPlaying = false; }
            else { music.play();  isPlaying = true; }
            updatePlayButton();
        }
        else if (btnPrev.contains(pos)) {
            currentIndex = (currentIndex - 1 + (int)currentPlaylist.size()) % currentPlaylist.size();
            playCurrent();
            updatePlayButton();
        }
        else if (btnNext.contains(pos)) {
            currentIndex = (currentIndex + 1) % currentPlaylist.size();
            playCurrent();
            updatePlayButton();
        }
    }

    void render() {
        window.clear(darkBg);

        float cx = W / 2.f;
        float topH = H * 0.70f;
        float pbX = 160.f;
        float pbW = W - pbX * 2.f;
        float pbY = topH + 28.f;

        window.draw(topPanel);
        window.draw(bottomPanel);

        CircleShape glow(165.f);
        glow.setOrigin({ 165.f, 165.f });
        glow.setPosition({ cx, 240.f });
        glow.setFillColor(Color(30, 215, 96, 12));    // green inner glow
        glow.setOutlineThickness(12.f);
        glow.setOutlineColor(Color(30, 215, 96, 30)); // green outer ring
        window.draw(glow);

        window.draw(disc);
        window.draw(discCenter);

        if (!currentPlaylist.empty()) {
            const Song& s = currentPlaylist[currentIndex];

            Text trackNum(font, to_string(currentIndex + 1) + " / " + to_string(currentPlaylist.size()), 15);
            trackNum.setFillColor(subText);
            FloatRect tnb = trackNum.getGlobalBounds();
            trackNum.setPosition({ cx - tnb.size.x / 2.f - tnb.position.x, 420.f });
            window.draw(trackNum);

            Text title(font, s.title, 28);
            title.setFillColor(textCol);
            FloatRect tb = title.getGlobalBounds();
            title.setPosition({ cx - tb.size.x / 2.f - tb.position.x, 448.f });
            window.draw(title);

            Text artist(font, s.artist, 18);
            artist.setFillColor(accent);
            FloatRect ab = artist.getGlobalBounds();
            artist.setPosition({ cx - ab.size.x / 2.f - ab.position.x, 484.f });
            window.draw(artist);

            Text moodTag(font, "[ " + s.mood + " ]", 14);
            moodTag.setFillColor(accent2);
            FloatRect mb = moodTag.getGlobalBounds();
            moodTag.setPosition({ cx - mb.size.x / 2.f - mb.position.x, 510.f });
            window.draw(moodTag);
        }

        window.draw(progressBarBg);
        window.draw(progressBarFill);
        window.draw(progressDot);

        if (!currentPlaylist.empty()) {
            float dur = music.getDuration().asSeconds();
            float cur = music.getPlayingOffset().asSeconds();

            auto toMMSS = [](float sec) -> string {
                int m = (int)sec / 60;
                int s = (int)sec % 60;
                return (m < 10 ? "0" : "") + to_string(m) + ":" + (s < 10 ? "0" : "") + to_string(s);
                };

            Text tCur(font, toMMSS(cur), 13);
            tCur.setFillColor(subText);
            tCur.setPosition({ pbX, pbY + 10.f });
            window.draw(tCur);

            Text tDur(font, toMMSS(dur), 13);
            tDur.setFillColor(subText);
            FloatRect db = tDur.getGlobalBounds();
            tDur.setPosition({ pbX + pbW - db.size.x - db.position.x, pbY + 10.f });
            window.draw(tDur);
        }

        btnPrev.draw(window);
        btnPlay.draw(window);
        btnNext.draw(window);
        btnBack.draw(window);

        RectangleShape divLine({ W, 1.f });
        divLine.setPosition({ 0.f, topH });
        divLine.setFillColor(Color(50, 50, 50));
        window.draw(divLine);

        window.display();
    }
};

// console setup . how much width and length for screen 
void centerConsole() {
    HWND hwnd = GetConsoleWindow();
    if (!hwnd) return;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT windowSize = { 0, 0, 119, 39 };
    SetConsoleWindowInfo(hOut, TRUE, &windowSize);
    COORD bufferSize = { 120, 40 };
    SetConsoleScreenBufferSize(hOut, bufferSize);
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int winW = rect.right - rect.left;
    int winH = rect.bottom - rect.top;
    MoveWindow(hwnd, (screenW - winW) / 2, (screenH - winH) / 2, winW, winH, TRUE);
}

// main function 
int main() {
    SetConsoleOutputCP(CP_UTF8);
    centerConsole();
    SetConsoleTitleA("Mood Music Player");
    system("color 0A");  // black bg + bright green text — color palette for terminal where music plays 

    vector<Song> database = {
        {"Levitating",    "Dua Lipa",                "Happy",     "Levitating.ogg"},
        {"Wildflower",    "Billie Eilish",            "Happy",     "wildflower.ogg"},
        {"Arcade",        "Duncan Laurence",          "Happy",     "Arcade.ogg"},
        {"Afreen Afreen", "Rahat Fateh Ali Khan",     "Sad",       "Afreen Afreen.ogg"},
        {"Tum Hi Aana",   "Jubin Nautiyal",           "Sad",       "Tum hi ana.ogg"},
        {"Perfect",       "Ed Sheeran",               "Sad",       "Perfect.ogg"},
        {"Believer",      "Imagine Dragons",          "Energetic", "Believer.ogg"},
        {"Ishq",          "Faheem Abdullah",          "Energetic", "Ishq.ogg"},
        {"Pasoori",       "Ali Sethi & Shae Gill",    "Energetic", "Pasoori.ogg"}
    };

    MusicPlayer player;
    int choice;

    system("cls");
    printBanner();

    do {
        printMenu();
        cin >> choice;
        cout << "\n";

        // exit 
        if (choice == 6) {
            system("cls");
            printBanner();
            setColor(4);
            cout << "                                                   Goodbye! See you next time!\n\n";
            resetColor();
            system("pause");
            break;
        }

        vector<Song> filtered;

        // filtering mood 
        if (choice >= 1 && choice <= 3) {
            string m = (choice == 1) ? "Happy" : (choice == 2) ? "Sad" : "Energetic";
            for (const auto& s : database)
                if (s.mood == m) filtered.push_back(s);

            if (filtered.empty()) {
                printError("No songs found for this mood.");
                system("pause");
                system("cls");
                printBanner();
                continue;
            }
        }
        // show all songs at once and then user can play it directly 
        else if (choice == 4) {
            filtered = database;
        }
        // add new song 
        else if (choice == 5) {
            system("cls");
            printBanner();
            string t, a, p, mood;
            cin.ignore();
            printDivider();
            setColor(14);
            cout << "                                                   ADD NEW SONG\n";
            resetColor();
            printDivider();
            setColor(15); cout << "                                                   Song Title  : "; resetColor(); getline(cin, t);
            setColor(15); cout << "                                                   Artist Name : "; resetColor(); getline(cin, a);
            setColor(15); cout << "                                                   Mood        : "; resetColor(); getline(cin, mood);
            setColor(15); cout << "                                                   File Name   : "; resetColor(); getline(cin, p);
            database.push_back(Song(t, a, mood, p));
            printSuccess("Song \"" + t + "\" added successfully!");
            system("pause");
            system("cls");
            printBanner();
            continue;
        }
        // for invalid function
        else {
            printError("Invalid choice. Please try again.");
            system("pause");
            system("cls");
            printBanner();
            continue;
        }

        // ── shows playlist and sub menus 
        system("cls");
        printBanner();
        vector<pair<string, string>> songPairs;
        for (auto& s : filtered) songPairs.push_back({ s.title, s.artist });
        printSongList(songPairs);
        printSubMenu();
        int sub; cin >> sub;
        cout << "\n";

        // play function
        if (sub == 1) {
            system("cls");
            printBanner();
            printDivider();
            setColor(14);
            cout << "                                               Which song do you want to play first?\n";
            printDivider();
            int i = 1;
            for (auto& s : filtered) {
                setColor(11); cout << "                                                " << i++ << ".  ";
                setColor(15); cout << s.title;
                setColor(8);  cout << "  --  ";
                setColor(10); cout << s.artist << "\n";
            }
            resetColor();
            printDivider();
            setColor(15);
            cout << "                                                 Enter song number (1 - " << filtered.size() << "): ";
            resetColor();

            int startIndex = 1;
            cin >> startIndex;
            cout << "\n";
            if (startIndex < 1 || startIndex >(int)filtered.size()) {
                printError("Invalid number. Starting from song 1.");
                startIndex = 1;
            }

            system("cls");
            printBanner();
            printSuccess("Now Playing: " + filtered[startIndex - 1].title +
                "  --  " + filtered[startIndex - 1].artist);
            system("pause");

            player.loadPlaylist(filtered, startIndex - 1);
            player.run();

            system("cls");
            printBanner();
        }
        // view songs 
        else if (sub == 2) {
            system("pause");
            system("cls");
            printBanner();
        }
        // for back menu 
        else if (sub == 3) {
            system("cls");
            printBanner();
        }
        else {
            printError("Invalid choice.");
            system("pause");
            system("cls");
            printBanner();
        }

    } while (choice != 6);

    return 0;
}