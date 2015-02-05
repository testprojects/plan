#ifndef UI_H
#define UI_H

class UI
{
public:
    UI();
    void deleteStream();
    void planStream();
private:
    void init();
    void showMainMenu();
    bool ShouldMoveStream();
    void terminate();
private:
    void clearScreen();
    void showMenu(int index);
};

#endif // UI_H
