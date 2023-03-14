#define EMPTY 0
#define P1    1
#define P2    2
#define HASH_SIZE 4194304
#define HASH_BITS 22

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <omp.h>
#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_popcount __popcnt
#pragma intrinsic(_BitScanForward)
#endif


typedef struct State State;
typedef struct Move Move;
typedef struct MCTSNode MCTSNode;
typedef struct ParentEdge ParentEdge;
typedef struct ChildEdge ChildEdge;

struct Move {
    int8_t x;
    int8_t y;
};

struct State {
    uint32_t p1_grid[3];
    uint32_t p2_grid[3];
    uint32_t p1_big_grid;
    uint32_t p2_big_grid;
    uint32_t valid_moves;
    int8_t turn;
    int8_t n_moves;
    int8_t winner;
};

State *new_game() {
    State *state = (State *) calloc(sizeof(State), 1);
    state->turn = P1;
    state->n_moves = 81;
    state->valid_moves = 0777;
    return state;
}

static bool win_lut[512] = {0};

void win_lut_init() {
    uint32_t patterns[8] = {0007, 0070, 0700, 0111, 0222, 0444, 0124, 0421};
    for (uint32_t i = 0; i < 512; i++) {
        for (int p = 0; p < 8; p++) {
            if (__builtin_popcount(i & patterns[p]) == 3) {
                win_lut[i] = true;
                break;
            }
        }
    }
}

void make_move(State *state, int action) {
    int index1, index2;
    unsigned int X, Y;
    unsigned int i = 0;
    uint32_t valid_moves = state->valid_moves;
    while (_BitScanForward(&index1, valid_moves)) {
        valid_moves &= ~(1 << index1);
        X = index1 / 3;
        Y = index1 % 3;
        uint32_t grid = ~(state->p1_grid[X] | state->p2_grid[X]);
        uint32_t mask = 0777 << (9 * Y);
        grid &= mask;
        while (_BitScanForward(&index2, grid)) {
            if (i == action) {
                goto L1;
            }
            grid &= ~(1 << index2);
            i++;
        }
    }
    L1:
    if (state->turn == P1) {
        state->p1_grid[X] |= 1 << index2;
        uint32_t grid = (state->p1_grid[X] >> (9 * Y)) & 0777;
        if (win_lut[grid]) {
            state->p1_big_grid |= 1 << index1;
            if (win_lut[state->p1_big_grid]) {
                state->winner = P1;
                state->n_moves = 0;
                state->turn = P2;
                return;
            }
        }
    } else {
        state->p2_grid[X] |= 1 << index2;
        uint32_t grid = (state->p2_grid[X] >> (9 * Y)) & 0777;
        if (win_lut[grid]) {
            state->p2_big_grid |= 1 << index1;
            if (win_lut[state->p2_big_grid]) {
                state->winner = P2;
                state->n_moves = 0;
                state->turn = P1;
                return;
            }
        }
    }

    uint32_t big_grid = state->p1_big_grid | state->p2_big_grid;
    i = index2 - (Y * 9);
    if ((big_grid & (1 << i)) == 0) {
        unsigned int x = i / 3;
        unsigned int y = i % 3;
        uint32_t grid = state->p1_grid[x] | state->p2_grid[x];
        uint32_t mask = 0777 << (9 * y);
        grid &= mask;
        state->n_moves = 9 - __builtin_popcount(grid);
        state->valid_moves = 1 << i;
    } else {
        int8_t n_moves = 0;

        for (int8_t X = 0; X < 3; X++) {
            uint32_t mask = (big_grid >> (X * 3)) & 07;
            mask = mask | (mask << 8) | (mask << 16);
            mask &= 01001001;
            mask = (mask << 9) - mask;
            uint32_t grid = state->p1_grid[X] | state->p2_grid[X];
            grid |= mask;
            n_moves += 27 - __builtin_popcount(grid);
        }

        state->n_moves = n_moves;
        state->valid_moves = (~big_grid) & 0777;
    }
    state->turn = state->turn == P1 ? P2 : P1;
    return;
}

State *copy_game(State *game) {
    State *game_copy = (State *) malloc(sizeof(State));
    memcpy(game_copy, game, sizeof(State));
    return game_copy;
}

struct ParentEdge {
    int8_t action;
    MCTSNode *parent;
    ParentEdge *next;
};

struct ChildEdge {
    MCTSNode *child;
    ChildEdge *next;
};

struct MCTSNode {
    State state;
    ParentEdge *parents;
    ChildEdge *children;
    int n;
    float q;
    int8_t moves_expanded;
    bool del;
};

/* <-- Hash stuff --> */
struct HashNode {
    MCTSNode *value;
    struct HashNode* next;
};

struct HashTable {
    struct HashNode* buckets[HASH_SIZE];
};

static struct HashTable hash_table;

void hash_init(struct HashTable* hash_table) {
    for (size_t i = 0; i < HASH_SIZE; i++) {
        hash_table->buckets[i] = NULL;
    }
}

uint32_t hash_func(const State* state) {
    uint32_t combined = 0;

    combined = (state->p1_grid[0] * 4024916473) ^ (state->p2_grid[0] * 2210760757);
    combined ^= (state->p1_grid[1] * 3801313199) ^ (state->p2_grid[1] * 3202149131);
    combined ^= (state->p1_grid[2] * 2679919601) ^ (state->p2_grid[2] * 356445256);

    combined ^= state->valid_moves;
    // Split the combined value into two halves using a bitmask
    uint32_t combined_lo = combined & (HASH_SIZE - 1);
    uint32_t combined_hi = combined >> (32 - HASH_BITS);
    uint32_t hash = combined_lo ^ combined_hi;
    return hash;
}

void hash_put(struct HashTable* hash_table, MCTSNode* mcts_node, uint32_t hash) {
    struct HashNode* node = (struct HashNode*) malloc(sizeof(struct HashNode));
    node->value = mcts_node;
    node->next = hash_table->buckets[hash];
    hash_table->buckets[hash] = node;
}

MCTSNode *hash_get(const struct HashTable* hash_table, const State* state, uint32_t hash) {
    struct HashNode* node = hash_table->buckets[hash];
    while (node != NULL) {
        bool match = true;
        for (size_t i = 0; i < 3; i++) {
            if (node->value->state.p1_grid[i] != state->p1_grid[i] ||
                node->value->state.p2_grid[i] != state->p2_grid[i]) {
                match = false;
                break;
            }
        }
        if (match && node->value->state.valid_moves == state->valid_moves) {
            return node->value;
        }
        node = node->next;
    }
    return 0;
}
/* end of hash stuff */


MCTSNode *best_action(MCTSNode *root, double time_limit, int num_rollouts, float c_param);
static float rollout(State *state);
static void backpropagate(MCTSNode *node, float reward);
static MCTSNode *tree_policy(MCTSNode *root, float c_param);
static MCTSNode *best_child(MCTSNode *node, float c_param);
static MCTSNode *expand(MCTSNode *node);
void delete_level(MCTSNode *node);
void delete_node(MCTSNode *node);

MCTSNode *best_action(MCTSNode *root, double time_limit, int num_rollouts, float c_param) {

    #pragma omp parallel firstprivate(root, time_limit, num_rollouts, c_param)
    {
    double start_time = omp_get_wtime();
    while (omp_get_wtime() - start_time < time_limit) {
        MCTSNode *v;
        #pragma omp critical
        {
            v = tree_policy(root, c_param);
        }
        float reward = 0;
        for (int r = 0; r < num_rollouts; r++) {
            reward += rollout(&(v->state));
        }
        reward = reward / num_rollouts;
        backpropagate(v, reward);
    }
    }
    return best_child(root, 0.0f);
}

static void backpropagate(MCTSNode *node, float reward) {
    #pragma omp atomic
    node->n++;

    #pragma omp atomic
    node->q += reward;

    for (ParentEdge *pa = node->parents; pa != 0; pa = pa->next) {
        backpropagate(pa->parent, 1.0f - reward);
    }
}

static MCTSNode *tree_policy(MCTSNode *root, float c_param) {
        MCTSNode *current_node = root;
        while (current_node->state.n_moves != 0) {
            if (current_node->moves_expanded < current_node->state.n_moves) {
                current_node = expand(current_node);
                break;
            }
            current_node = best_child(current_node, c_param);
        }
        return current_node;
}

static MCTSNode *best_child(MCTSNode *node, float c_param) {
    if (node->n <= 0) {
        return node->children->child;
    }
    MCTSNode *best_child = 0;
    float best_score = -INFINITY;
    float log_n = logf((float) node->n);
    for (ChildEdge *edge = node->children; edge != 0; edge = edge->next) {
        float q = edge->child->q;
        float n = (float) edge->child->n;
        if (n <= 0) {
            q = 0.0f;
            n = 1.0f;
        }
        float score = (q / n) + c_param * sqrtf(log_n / n);
        if (score > best_score) {
            best_score = score;
            best_child = edge->child;
        }
    }
    return best_child;
}

static MCTSNode *expand(MCTSNode *node) {
        int action = node->moves_expanded;
        State state = node->state;
        make_move(&state, action);
        uint32_t hash = hash_func(&state);
        MCTSNode *child_node = hash_get(&hash_table, &state, hash);
        if (child_node == 0) {
            child_node = (MCTSNode *) malloc(sizeof(MCTSNode));
            child_node->state = state;
            child_node->parents = 0;
            child_node->children = 0;
            child_node->n = 0;
            child_node->q = 0;
            child_node->moves_expanded = 0;
            hash_put(&hash_table, child_node, hash);
        }

        ParentEdge *pe = (ParentEdge *) malloc(sizeof(ParentEdge));
        pe->action = node->moves_expanded;
        pe->parent = node;
        pe->next = child_node->parents;
        child_node->parents = pe;

        ChildEdge *ce = (ChildEdge *) malloc(sizeof(ChildEdge));
        ce->child = child_node;
        ce->next = node->children;
        node->children = ce;

        node->moves_expanded += 1;
        return child_node;
}

static float rollout(State *state) {
    State current_rollout_state = *state;
    while (current_rollout_state.n_moves != 0) {
        int action = rand() % current_rollout_state.n_moves;
        make_move(&current_rollout_state, action);
    }
    float result;
    if (current_rollout_state.winner == 0) {
        result = 0.5f;
    } else if (current_rollout_state.winner == state->turn) {
        result = 0.0f;
    } else {
        result = 1.0f;
    }
    return result;
}


void delete_level(MCTSNode *node) {
    for (ChildEdge *child_edge = node->children; child_edge != 0; child_edge = child_edge->next) {
        for (ParentEdge *parent_edge = child_edge->child->parents; parent_edge != 0; parent_edge = parent_edge->next) {
            free(parent_edge);
        }
        child_edge->child->parents = 0;
        free(child_edge);
    }
    node->children = 0;
}


void flag_non_deletable_nodes(MCTSNode *root) {
    if (root->del == 0) {
        return;
    }
    root->del = 0;
    for (ChildEdge *edge = root->children; edge != 0; edge = edge->next) {
        flag_non_deletable_nodes(edge->child);
    }
}


MCTSNode *update_root(struct HashTable* table, State *state) {
    int i;
    #pragma omp parallel for
    for (i = 0; i < HASH_SIZE; i++) {
        struct HashNode* node = table->buckets[i];
        while (node != 0) {
            MCTSNode* value = node->value;
            value->del = 1;
            // Move to the next node in the linked list
            node = node->next;
        }
    }

    uint32_t hash = hash_func(state);
    MCTSNode *root = hash_get(&hash_table, state, hash);

    if (root) {
        flag_non_deletable_nodes(root);
    } else {
        root = (MCTSNode *) calloc(sizeof(MCTSNode), 1);
        root->state = *state;
        root->parents = 0;
        root->children = 0;
        root->n = 0;
        root->q = 0;
        root->moves_expanded = 0;
    }

    #pragma omp parallel for
    for (i = 0; i < HASH_SIZE; i++) {
        struct HashNode* node = table->buckets[i];
        while (node != 0) {
            MCTSNode* value = node->value;
            ParentEdge *prev = 0;
            for (ParentEdge *edge = value->parents; edge != 0; ) {
                ParentEdge *next = edge->next;  // Store next pointer before freeing current node
                if (edge->parent->del) {
                    free(edge);
                    if (prev) {
                        prev->next = next;
                    } else {
                        value->parents = next;
                    }
                } else {
                    prev = edge;
                }
                edge = next;
            }
            // Move to the next node in the linked list
            node = node->next;
        }
    }

    #pragma omp parallel for
    for (i = 0; i < HASH_SIZE; i++) {
        struct HashNode* node = table->buckets[i];
        struct HashNode* prev = NULL;
        while (node != 0) {
            MCTSNode* value = node->value;
            struct HashNode* next_node = node->next;
            if (value->del) {
                for (ChildEdge *edge = value->children; edge != 0; ) {
                    ChildEdge *next = edge->next;  // Store next pointer before freeing current node
                    free(edge);
                    edge = next;
                }
                for (ParentEdge *edge = value->parents; edge != 0; ) {
                    ParentEdge *next = edge->next;  // Store next pointer before freeing current node
                    free(edge);
                    edge = next;
                }
                free(value);
                free(node);
                // Update the previous node's next pointer to skip the deleted node
                if (prev) {
                    prev->next = next_node;
                } else {
                    table->buckets[i] = next_node;
                }
            } else {
                prev = node;
            }
            node = next_node;
        }
    }

    return root;
}

