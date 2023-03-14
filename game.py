import copy

class Game:
    EMPTY = 0
    P1 = 1
    P2 = 2

    def __init__(self):
        self.grid = [[[[Game.EMPTY]*3 for _ in range(3)] for _ in range(3)] for _ in range(3)]
        self.big_grid = [[Game.EMPTY]*3 for _ in range(3)]
        self.turn = Game.P1
        self.valid_moves = [(x, y) for x in range(9) for y in range(9)]
        self.winner = -1

    def place_tile(self, x, y):

        if (x, y) not in self.valid_moves:
            return False

        X = x // 3
        Y = y // 3
        x = x % 3
        y = y % 3

        subgrid = self.grid[X][Y]

        subgrid[x][y] = self.turn
        if Game.check_win(subgrid, x, y):
            self.big_grid[X][Y] = self.turn
            if Game.check_win(self.big_grid, X, Y):
                self.winner = self.turn
                self.valid_moves = []
                return True

        if self.big_grid[x][y] == Game.EMPTY:
            subgrid = self.grid[x][y]
            self.valid_moves = [(3 * x + _x, 3 * y + _y) for _x in range(3) for _y in range(3) if
                                subgrid[_x][_y] == Game.EMPTY]
        else:
            self.valid_moves = []
            for x in range(3):
                for y in range(3):
                    if self.big_grid[x][y] == Game.EMPTY:
                        subgrid = self.grid[x][y]
                        self.valid_moves.extend([(3 * x + _x, 3 * y + _y) for _x in range(3) for _y in range(3) if
                                                 subgrid[_x][_y] == Game.EMPTY])

        self.turn = Game.P2 if self.turn == Game.P1 else Game.P1
        return True

    def copy(self):
        game_copy = Game()
        game_copy.grid = copy.deepcopy(self.grid)
        game_copy.big_grid = copy.deepcopy(self.big_grid)
        game_copy.turn = self.turn
        game_copy.valid_moves = copy.deepcopy(self.valid_moves)
        game_copy.winner = self.winner
        return game_copy

    @staticmethod
    def check_win(grid, x, y):
        tile = grid[x][y]
        if all(tile == grid[x][y_] for y_ in range(3)) or all(tile == grid[x_][y] for x_ in range(3)):
            return True
        # diagonal
        if x == y and all(tile == grid[i][i] for i in range(3)):
            return True
        if x % 2 == y % 2 and all(tile == grid[_x][_y] for _x, _y in ((0, 2), (1, 1), (2, 0))):
            return True
        return False
