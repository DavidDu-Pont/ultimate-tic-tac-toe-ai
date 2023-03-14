import game
from tkinter import *
import time

try:
    from c_module import myModule
except ImportError:
    # compile source files
    from c_module import setup
    from c_module import myModule


SIZE = 5

master = Tk()

canvas_size = 500
cell_size = canvas_size/9
w = Canvas(master, width=canvas_size, height=canvas_size + 100)
w.pack()

grid = [[0]*9 for _ in range(9)]
big_grid = [[0]*3 for _ in range(3)]

turn = 1

text = w.create_text(250, canvas_size + 50, text=f"P{turn}", fill="black", font=('grandview 20 bold'))

game = myModule.newGame()
valid_moves = myModule.getAllMoves(game)


def draw_grid():
    s = cell_size
    for x in range(9):
        for y in range(9):
            grid[x][y] = w.create_rectangle(s * x, s * y, s * (x + 1), s * (y + 1), fill="", width=1)

    s = 3*cell_size
    for x in range(3):
        for y in range(3):
            big_grid[x][y] = w.create_rectangle(s * x, s * y, s * (x + 1), s * (y + 1), fill="", width=3)


def click(event):
    global turn, valid_moves

    s = cell_size
    x = int(event.x / s)
    y = int(event.y / s)

    if not (0 <= x < 9 and 0 <= y < 9):
        return

    if (x, y) not in valid_moves:
        return

    for x_ in range(9):
        for y_ in range(9):
            if w.itemcget(grid[x_][y_], "fill") == '#f7f09c':
                w.itemconfig(grid[x_][y_], fill='')

    myModule.makeMove(game, valid_moves.index((x, y)))
    valid_moves = myModule.getAllMoves(game)

    if turn == 1:
        w.itemconfig(grid[x][y], fill='#a7ed87')
        turn = 2
    else:
        w.itemconfig(grid[x][y], fill='#4ab4cf')
        turn = 1

    w.itemconfig(text, text=f"P{turn}")

    for move in valid_moves:
        w.itemconfig(grid[move[0]][move[1]], fill='#f7f09c')

    #w.update()
    #click_right(event)


def click_right(event):
    global turn, valid_moves

    if not valid_moves:
        return

    for x_ in range(9):
        for y_ in range(9):
            if w.itemcget(grid[x_][y_], "fill") == '#f7f09c':
                w.itemconfig(grid[x_][y_], fill='')
    t0 = time.time()
    best_action = myModule.getBestMove(game, 1, 5, 1.41)
    print("time: ", time.time() - t0)
    x, y = valid_moves[best_action]
    myModule.makeMove(game, best_action)
    valid_moves = myModule.getAllMoves(game)

    if turn == 1:
        w.itemconfig(grid[x][y], fill='#a7ed87')
        turn = 2
    else:
        w.itemconfig(grid[x][y], fill='#4ab4cf')
        turn = 1

    w.itemconfig(text, text=f"P{turn}")

    for move in valid_moves:
        w.itemconfig(grid[move[0]][move[1]], fill='#f7f09c')



draw_grid()
w.focus_set()
w.bind('<Button-1>', click)
w.bind('<Button-3>', click_right)


mainloop()
