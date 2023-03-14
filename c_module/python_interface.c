#include <Python.h>
#include "game.c"


static PyObject* new_game_py(PyObject* self, PyObject* args) {

    struct Game *game = new_game();

    return PyLong_FromUnsignedLongLong((unsigned long long) game);
}

static PyObject* get_moves_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    PyArg_ParseTuple(args, "K", &ptr_ull);
    struct Game *game = (struct Game *) ptr_ull;

    return PyLong_FromLong((long) game->n_moves);
}

static PyObject* get_all_moves_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    PyArg_ParseTuple(args, "K", &ptr_ull);
    struct Game *game = (struct Game *) ptr_ull;

    PyObject *list = PyList_New(game->n_moves);
    for (int i = 0; i < game->n_moves; i++) {
        PyList_SET_ITEM(list, i, Py_BuildValue("ii", (int) game->valid_moves[i].x, (int) game->valid_moves[i].y));
    }

    return list;
}

static PyObject* get_board_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    PyArg_ParseTuple(args, "K", &ptr_ull);
    struct Game *game = (struct Game *) ptr_ull;

    PyObject *list = PyList_New(81 + 9 + 9);
    int i = 0;
    for (int X = 0; X < 3; X++) {
        for (int Y = 0; Y < 3; Y++) {
            for (int x = 0; x < 3; x++) {
                for (int y = 0; y < 3; y++) {
                    PyList_SET_ITEM(list, i++, PyLong_FromLong((long) game->grid[X][Y][x][y]));
                }
            }
        }
    }
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            PyList_SET_ITEM(list, i++, PyLong_FromLong((long) game->big_grid[x][y]));
        }
    }
    long temp_grid[3][3] = {0};
    for (int x = 0; x < game->n_moves; x++) {
        temp_grid[game->valid_moves[x].x / 3][game->valid_moves[x].y / 3] = game->turn;
    }
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            PyList_SET_ITEM(list, i++, PyLong_FromLong((long) temp_grid[x][y]));
        }
    }

    return list;
}

static PyObject* get_child_boards_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    PyArg_ParseTuple(args, "K", &ptr_ull);
    struct Game *game = (struct Game *) ptr_ull;

    PyObject *all_boards = PyList_New(game->n_moves);

    for (int w = 0; w < game->n_moves; ++w) {
        struct Game *game_copy = copy_game(game);
        place_tile(game_copy, game_copy->valid_moves[w].x, game_copy->valid_moves[w].y);

        PyObject *list = PyList_New(81 + 9 + 9);
        int i = 0;
        for (int X = 0; X < 3; X++) {
            for (int Y = 0; Y < 3; Y++) {
                for (int x = 0; x < 3; x++) {
                    for (int y = 0; y < 3; y++) {
                        PyList_SET_ITEM(list, i++, PyLong_FromLong((long) game_copy->grid[X][Y][x][y]));
                    }
                }
            }
        }
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                PyList_SET_ITEM(list, i++, PyLong_FromLong((long) game_copy->big_grid[x][y]));
            }
        }
        long temp_grid[3][3] = {0};
        for (int x = 0; x < game->n_moves; x++) {
            temp_grid[game->valid_moves[x].x / 3][game->valid_moves[x].y / 3] = game->turn;
        }
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                PyList_SET_ITEM(list, i++, PyLong_FromLong((long) temp_grid[x][y]));
            }
        }

        PyList_SET_ITEM(all_boards, w, list);

        free(game_copy);
    }

    return all_boards;
}

static PyObject* get_symmetries_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    PyArg_ParseTuple(args, "K", &ptr_ull);
    struct Game *game = (struct Game *) ptr_ull;

    PyObject *all_boards = PyList_New(8);
    PyObject *list0 = PyList_New(81 + 9 + 9);
    PyObject *list1 = PyList_New(81 + 9 + 9);
    PyObject *list2 = PyList_New(81 + 9 + 9);
    PyObject *list3 = PyList_New(81 + 9 + 9);
    PyObject *list4 = PyList_New(81 + 9 + 9);
    PyObject *list5 = PyList_New(81 + 9 + 9);
    PyObject *list6 = PyList_New(81 + 9 + 9);
    PyObject *list7 = PyList_New(81 + 9 + 9);
    int i = 0;
    for (int X = 0; X < 3; X++) {
        for (int Y = 0; Y < 3; Y++) {
            for (int x = 0; x < 3; x++) {
                for (int y = 0; y < 3; y++) {
                    PyList_SET_ITEM(list0, i, PyLong_FromLong((long) game->grid[X][Y][x][y]));
                    PyList_SET_ITEM(list1, i, PyLong_FromLong((long) game->grid[2-X][Y][2-x][y]));
                    PyList_SET_ITEM(list2, i, PyLong_FromLong((long) game->grid[X][2-Y][x][2-y]));
                    PyList_SET_ITEM(list3, i, PyLong_FromLong((long) game->grid[2-X][2-Y][2-x][2-y]));
                    PyList_SET_ITEM(list4, i, PyLong_FromLong((long) game->grid[Y][X][y][x]));
                    PyList_SET_ITEM(list5, i, PyLong_FromLong((long) game->grid[Y][2-X][y][2-x]));
                    PyList_SET_ITEM(list6, i, PyLong_FromLong((long) game->grid[2-Y][X][2-y][x]));
                    PyList_SET_ITEM(list7, i, PyLong_FromLong((long) game->grid[2-Y][2-X][2-y][2-x]));
                    i += 1;
                }
            }
        }
    }
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            PyList_SET_ITEM(list0, i, PyLong_FromLong((long) game->big_grid[x][y]));
            PyList_SET_ITEM(list1, i, PyLong_FromLong((long) game->big_grid[2-x][y]));
            PyList_SET_ITEM(list2, i, PyLong_FromLong((long) game->big_grid[x][2-y]));
            PyList_SET_ITEM(list3, i, PyLong_FromLong((long) game->big_grid[2-x][2-y]));
            PyList_SET_ITEM(list4, i, PyLong_FromLong((long) game->big_grid[y][x]));
            PyList_SET_ITEM(list5, i, PyLong_FromLong((long) game->big_grid[y][2-x]));
            PyList_SET_ITEM(list6, i, PyLong_FromLong((long) game->big_grid[2-y][x]));
            PyList_SET_ITEM(list7, i, PyLong_FromLong((long) game->big_grid[2-y][2-x]));
            i += 1;
        }
    }
    long temp_grid[3][3] = {0};
    for (int x = 0; x < game->n_moves; x++) {
        temp_grid[game->valid_moves[x].x / 3][game->valid_moves[x].y / 3] = game->turn;
    }
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            PyList_SET_ITEM(list0, i, PyLong_FromLong((long) temp_grid[x][y]));
            PyList_SET_ITEM(list1, i, PyLong_FromLong((long) temp_grid[2-x][y]));
            PyList_SET_ITEM(list2, i, PyLong_FromLong((long) temp_grid[x][2-y]));
            PyList_SET_ITEM(list3, i, PyLong_FromLong((long) temp_grid[2-x][2-y]));
            PyList_SET_ITEM(list4, i, PyLong_FromLong((long) temp_grid[y][x]));
            PyList_SET_ITEM(list5, i, PyLong_FromLong((long) temp_grid[y][2-x]));
            PyList_SET_ITEM(list6, i, PyLong_FromLong((long) temp_grid[2-y][x]));
            PyList_SET_ITEM(list7, i, PyLong_FromLong((long) temp_grid[2-y][2-x]));
            i += 1;
        }
    }

    PyList_SET_ITEM(all_boards, 0, list0);
    PyList_SET_ITEM(all_boards, 1, list1);
    PyList_SET_ITEM(all_boards, 2, list2);
    PyList_SET_ITEM(all_boards, 3, list3);
    PyList_SET_ITEM(all_boards, 4, list4);
    PyList_SET_ITEM(all_boards, 5, list5);
    PyList_SET_ITEM(all_boards, 6, list6);
    PyList_SET_ITEM(all_boards, 7, list7);

    return all_boards;
}

static PyObject* make_move_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    int move_num;
    PyArg_ParseTuple(args, "Ki", &ptr_ull, &move_num);

    struct Game *game = (struct Game *) ptr_ull;

    int8_t x = game->valid_moves[move_num].x;
    int8_t y = game->valid_moves[move_num].y;

    bool return_val = place_tile(game, x, y);
    return PyBool_FromLong((long) return_val);
}

static PyObject* get_winner_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    PyArg_ParseTuple(args, "K", &ptr_ull);

    struct Game *game = (struct Game *) ptr_ull;

    return PyLong_FromLong((long) game->winner);
}

static PyObject* copy_game_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    PyArg_ParseTuple(args, "K", &ptr_ull);

    struct Game *game = (struct Game *) ptr_ull;

    struct Game *game_copy = copy_game(game);

    return PyLong_FromUnsignedLongLong((unsigned long long) game_copy);
}

static PyObject* free_game_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    PyArg_ParseTuple(args, "K", &ptr_ull);

    struct Game *game = (struct Game *) ptr_ull;
    free(game);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* print_board_py(PyObject* self, PyObject* args) {
    unsigned long long ptr_ull;
    PyArg_ParseTuple(args, "K", &ptr_ull);

    struct Game *game = (struct Game *) ptr_ull;

    for (int y = 0; y < 9; y++) {
        for (int x = 0; x < 9; x++) {
            printf("%i ", game->grid[x/3][y/3][x%3][y%3]);
        }
        printf("\n");
    }
    printf("######################################\n");
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            printf("%i ", game->big_grid[x][y]);
        }
        printf("\n");
    }
    printf("######################################\n");

    return PyLong_FromUnsignedLongLong((unsigned long long) game);
}

// Our Module's Function Definition struct
// We require this `NULL` to signal the end of our method
// definition
static PyMethodDef myMethods[] = {
    { "newGame", new_game_py, METH_VARARGS, "Start a new game."},
    { "getMoves", get_moves_py, METH_VARARGS, "Get the number of moves."},
    { "makeMove", make_move_py, METH_VARARGS, "Make a move."},
    { "getWinner", get_winner_py, METH_VARARGS, "Print the board."},
    { "copyGame", copy_game_py, METH_VARARGS, "Copy the game."},
    { "printBoard", print_board_py, METH_VARARGS, "Print board."},
    { "freeGame", free_game_py, METH_VARARGS, "Free memory."},
    { "getAllMoves", get_all_moves_py, METH_VARARGS, "Free memory."},
    { "getBoard", get_board_py, METH_VARARGS, "Free memory."},
    { "getSymmetries", get_symmetries_py, METH_VARARGS, "Free memory."},
    { "getChildBoards", get_child_boards_py, METH_VARARGS, "Free memory."},
    { NULL, NULL, 0, NULL }
};

// Our Module Definition struct
static struct PyModuleDef myModule = {
    PyModuleDef_HEAD_INIT,
    "myModule",
    "Test Module",
    -1,
    myMethods
};

// Initializes our module using our above struct
PyMODINIT_FUNC PyInit_myModule(void)
{
    return PyModule_Create(&myModule);
}