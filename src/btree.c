#include "btree.h"

#include <stdlib.h>

static BTreeNode *btree_create_node(int is_leaf);
static void btree_free_node(BTreeNode *node);
static int btree_search_node(const BTreeNode *node, int key, long *value_out);
static int btree_split_child(BTreeNode *parent, int index);
static int btree_insert_nonfull(BTreeNode *node, int key, long value);
static int btree_get_max_node(const BTreeNode *node, int *key_out, long *value_out);

/* btree_init()은 B-tree 인덱스를 빈 상태로 시작하게 만든다. */
void btree_init(BTreeIndex *tree) {
    /* BTreeIndex는 루트 노드 하나만 들고 시작한다.
       처음에는 아무 데이터도 없으므로 root를 NULL로 둔다. */
    if (tree == NULL) {
        return;
    }

    tree->root = NULL;
}

/* btree_free()는 루트부터 모든 노드를 재귀적으로 해제한다. */
void btree_free(BTreeIndex *tree) {
    /* 트리 전체를 재귀적으로 순회하며 노드를 모두 free한다. */
    if (tree == NULL) {
        return;
    }

    btree_free_node(tree->root);
    tree->root = NULL;
}

/* btree_search()는 주어진 key가 트리에 있는지 찾고,
   있으면 연결된 value(이 프로젝트에서는 파일 오프셋)를 돌려준다. */
int btree_search(const BTreeIndex *tree, int key, long *value_out) {
    /* 빈 트리에서는 찾을 수 있는 key가 없으므로 바로 0을 반환한다. */
    if (tree == NULL || tree->root == NULL) {
        return 0;
    }

    return btree_search_node(tree->root, key, value_out);
}

/* btree_insert()는 새 key/value를 트리에 넣는다.
   필요하면 꽉 찬 노드를 분할해 B-tree 규칙을 유지한다. */
int btree_insert(BTreeIndex *tree, int key, long value) {
    /* root는 현재 트리의 시작 노드,
       new_root는 기존 루트가 꽉 찼을 때 새로 생기는 상위 루트다. */
    BTreeNode *root;
    BTreeNode *new_root;
    int status;

    if (tree == NULL) {
        return -1;
    }

    if (tree->root == NULL) {
        /* 첫 삽입이라면 루트 노드 하나를 leaf로 만들어 바로 저장한다. */
        tree->root = btree_create_node(1);
        if (tree->root == NULL) {
            return -1;
        }
        tree->root->keys[0] = key;
        tree->root->values[0] = value;
        tree->root->key_count = 1;
        return 0;
    }

    if (btree_search(tree, key, NULL)) {
        /* 이 프로젝트에서는 id 중복을 허용하지 않으므로
           이미 같은 key가 있으면 별도 상태 1을 돌려준다. */
        return 1;
    }

    root = tree->root;
    if (root->key_count == BTREE_MAX_KEYS) {
        /* 루트가 꽉 찼다면 더 아래로 넣기 전에
           한 단계 위에 새 루트를 만들고 기존 루트를 둘로 나눈다.
           그래야 삽입할 공간이 생긴다. */
        new_root = btree_create_node(0);
        if (new_root == NULL) {
            return -1;
        }

        new_root->children[0] = root;
        tree->root = new_root;

        /* 새 루트의 0번 자식(기존 root)을 분할한다. */
        status = btree_split_child(new_root, 0);
        if (status != 0) {
            btree_free_node(new_root);
            tree->root = root;
            return -1;
        }

        return btree_insert_nonfull(new_root, key, value);
    }

    return btree_insert_nonfull(root, key, value);
}

/* btree_get_max()는 현재 트리 안에서 가장 큰 key를 찾는다.
   auto-increment id 계산에 사용된다. */
int btree_get_max(const BTreeIndex *tree, int *key_out, long *value_out) {
    if (tree == NULL || tree->root == NULL) {
        return 0;
    }

    return btree_get_max_node(tree->root, key_out, value_out);
}

/* btree_create_node()는 새 B-tree 노드 한 개를 heap에 만든다. */
static BTreeNode *btree_create_node(int is_leaf) {
    BTreeNode *node;

    /* calloc을 사용하면 keys/values/children가 0으로 초기화되어
       초보자 입장에서 "쓰레기값" 문제를 줄일 수 있다. */
    node = (BTreeNode *) calloc(1, sizeof(BTreeNode));
    if (node == NULL) {
        return NULL;
    }

    /* leaf면 자식이 없는 말단 노드, 0이면 내부 노드다. */
    node->is_leaf = is_leaf;
    return node;
}

/* btree_free_node()는 한 노드와 그 아래 자식 트리를 재귀적으로 모두 해제한다. */
static void btree_free_node(BTreeNode *node) {
    /* i는 현재 free할 자식 번호를 가리킨다. */
    int i;

    if (node == NULL) {
        return;
    }

    if (!node->is_leaf) {
        /* 내부 노드는 자기 자신을 free하기 전에
           아래 자식들을 먼저 모두 free해야 한다. */
        for (i = 0; i <= node->key_count; i++) {
            btree_free_node(node->children[i]);
        }
    }

    free(node);
}

/* btree_search_node()는 현재 노드 기준으로
   key가 들어 있을 위치를 찾고, 필요하면 아래 자식으로 내려간다. */
static int btree_search_node(const BTreeNode *node, int key, long *value_out) {
    /* i는 현재 node 안에서 key가 들어갈 위치를 찾는 인덱스다. */
    int i = 0;

    /* keys는 오름차순 정렬되어 있으므로
       찾는 key보다 작은 동안만 오른쪽으로 이동한다. */
    while (i < node->key_count && key > node->keys[i]) {
        i++;
    }

    if (i < node->key_count && key == node->keys[i]) {
        /* 현재 노드 안에서 정확히 같은 key를 찾은 경우다. */
        if (value_out != NULL) {
            *value_out = node->values[i];
        }
        return 1;
    }

    if (node->is_leaf) {
        /* leaf까지 왔는데 못 찾았으면 트리 전체에도 없는 key다. */
        return 0;
    }

    /* 아직 leaf가 아니면
       key가 들어가야 할 자식 쪽으로 한 단계 더 내려간다. */
    return btree_search_node(node->children[i], key, value_out);
}

/* btree_split_child()는 꽉 찬 자식 노드를 둘로 나누고,
   가운데 key를 부모로 올려 B-tree 형태를 유지한다. */
static int btree_split_child(BTreeNode *parent, int index) {
    /* child는 꽉 찬 자식 노드,
       sibling은 분할 후 오른쪽 절반을 담을 새 노드다. */
    BTreeNode *child = parent->children[index];
    BTreeNode *sibling;
    /* i는 배열을 뒤에서 앞으로 밀거나 복사할 때 사용한다. */
    int i;

    sibling = btree_create_node(child->is_leaf);
    if (sibling == NULL) {
        return -1;
    }

    /* B-tree 분할 규칙상, 새 형제 노드는 절반 크기의 key를 받는다. */
    sibling->key_count = BTREE_MIN_DEGREE - 1;

    for (i = 0; i < BTREE_MIN_DEGREE - 1; i++) {
        /* child의 오른쪽 절반 key/value를 sibling으로 복사한다. */
        sibling->keys[i] = child->keys[i + BTREE_MIN_DEGREE];
        sibling->values[i] = child->values[i + BTREE_MIN_DEGREE];
    }

    if (!child->is_leaf) {
        for (i = 0; i < BTREE_MIN_DEGREE; i++) {
            /* 내부 노드라면 자식 포인터도 절반을 떼어 새 형제로 옮긴다. */
            sibling->children[i] = child->children[i + BTREE_MIN_DEGREE];
            child->children[i + BTREE_MIN_DEGREE] = NULL;
        }
    }

    /* child는 왼쪽 절반만 남기므로 key 개수를 줄인다. */
    child->key_count = BTREE_MIN_DEGREE - 1;

    for (i = parent->key_count; i >= index + 1; i--) {
        /* parent에 새 자식이 끼어들 자리를 만들기 위해
           기존 자식 포인터들을 한 칸씩 오른쪽으로 민다. */
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[index + 1] = sibling;

    for (i = parent->key_count - 1; i >= index; i--) {
        /* parent의 key 배열도 같은 이유로 한 칸씩 뒤로 민다. */
        parent->keys[i + 1] = parent->keys[i];
        parent->values[i + 1] = parent->values[i];
    }

    /* child 가운데 key 하나가 parent로 올라간다.
       이 key가 왼쪽/오른쪽 형제를 가르는 기준점이 된다. */
    parent->keys[index] = child->keys[BTREE_MIN_DEGREE - 1];
    parent->values[index] = child->values[BTREE_MIN_DEGREE - 1];
    parent->key_count++;

    return 0;
}

/* btree_insert_nonfull()은 아직 꽉 차지 않은 노드 쪽으로 내려가
   새 key/value를 실제 자리에 삽입하는 함수다. */
static int btree_insert_nonfull(BTreeNode *node, int key, long value) {
    /* i는 삽입 위치를 찾기 위해 오른쪽에서 왼쪽으로 움직이는 인덱스다. */
    int i = node->key_count - 1;
    int status;

    if (node->is_leaf) {
        while (i >= 0 && key < node->keys[i]) {
            /* 빈 자리를 만들기 위해 더 큰 key들을 한 칸씩 뒤로 민다. */
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }

        /* i가 멈춘 자리 바로 오른쪽이 새 key가 들어갈 위치다. */
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->key_count++;
        return 0;
    }

    while (i >= 0 && key < node->keys[i]) {
        i--;
    }
    /* while이 끝난 시점의 i는 "작거나 같은 마지막 위치"이므로
       실제 내려갈 자식 번호는 그 오른쪽인 i + 1이다. */
    i++;

    if (node->children[i]->key_count == BTREE_MAX_KEYS) {
        /* 내려가려는 자식이 꽉 찼다면 미리 분할해서 공간을 확보한다. */
        status = btree_split_child(node, i);
        if (status != 0) {
            return -1;
        }

        if (key > node->keys[i]) {
            /* 분할 후에는 부모에 새 기준 key가 생기므로
               삽입 대상이 오른쪽 형제로 넘어갈 수 있다. */
            i++;
        }
    }

    return btree_insert_nonfull(node->children[i], key, value);
}

/* btree_get_max_node()는 오른쪽 끝 leaf까지 내려가
   그 트리에서 가장 큰 key/value를 찾는다. */
static int btree_get_max_node(const BTreeNode *node, int *key_out, long *value_out) {
    /* cursor는 "가장 큰 key는 항상 가장 오른쪽에 있다"는 성질을 이용해
       트리의 오른쪽 끝 leaf까지 내려가기 위해 사용한다. */
    const BTreeNode *cursor = node;

    if (cursor == NULL) {
        return 0;
    }

    while (!cursor->is_leaf) {
        /* 내부 노드에서는 가장 오른쪽 자식으로 계속 내려간다. */
        cursor = cursor->children[cursor->key_count];
    }

    if (cursor->key_count <= 0) {
        return 0;
    }

    if (key_out != NULL) {
        /* leaf의 마지막 key가 이 트리 전체 최대 key다. */
        *key_out = cursor->keys[cursor->key_count - 1];
    }
    if (value_out != NULL) {
        *value_out = cursor->values[cursor->key_count - 1];
    }

    return 1;
}
