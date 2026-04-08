#ifndef BTREE_H
#define BTREE_H

#define BTREE_MIN_DEGREE 2
#define BTREE_MAX_KEYS ((2 * BTREE_MIN_DEGREE) - 1)
#define BTREE_MAX_CHILDREN (2 * BTREE_MIN_DEGREE)

typedef struct BTreeNode {
    int key_count;
    int is_leaf;
    int keys[BTREE_MAX_KEYS];
    long values[BTREE_MAX_KEYS];
    struct BTreeNode *children[BTREE_MAX_CHILDREN];
} BTreeNode;

typedef struct {
    BTreeNode *root;
} BTreeIndex;

void btree_init(BTreeIndex *tree);
void btree_free(BTreeIndex *tree);
int btree_search(const BTreeIndex *tree, int key, long *value_out);
int btree_insert(BTreeIndex *tree, int key, long value);
int btree_get_max(const BTreeIndex *tree, int *key_out, long *value_out);

#endif
