#include "path.h"

Node *__pathNode_eval(PathNode *pn, Node *n, PathError *err) {
    *err = E_OK;
    if (!n) {
        goto badtype;
    }

    if (n->type == N_ARRAY) {
        Node *rn = NULL;
        if (NT_INDEX == pn->type) {
            int index = pn->value.index;
            if (index < 0) index = Node_Length(n) + index;
            int rc = Node_ArrayItem(n, index, &rn);
            if (rc != OBJ_OK) {
                *err = E_NOINDEX;
            }
        } else if (NT_INFINITE == pn->type) {
            *err = E_INFINDEX;
        } else {
            goto badtype;
        }
        return rn;
    }

    if (n->type == N_DICT) {
        if (pn->type != NT_KEY) {
            goto badtype;
        }
        Node *rn = NULL;
        int rc = Node_DictGet(n, pn->value.key, &rn);
        if (rc != OBJ_OK) {
            *err = E_NOKEY;
        }
        return rn;
    }

badtype:
    *err = E_BADTYPE;
    return NULL;
}

PathError SearchPath_Find(SearchPath *path, Node *root, Node **n) {
    Node *current = root;
    PathError ret;
    for (int i = 0; i < path->len; i++) {
        current = __pathNode_eval(&path->nodes[i], current, &ret);
        if (ret != E_OK) {
            *n = NULL;
            return ret;
        }
    }
    *n = current;
    return E_OK;
}

PathError SearchPath_FindEx(SearchPath *path, Node *root, Node **n, Node **p, int *errnode) {
    Node *current = root;
    Node *prev = NULL;
    Node *next;
    PathError ret;

    for (int i = 0; i < path->len; i++) {
        prev = current;
        current = __pathNode_eval(&path->nodes[i], current, &ret);
        if (ret != E_OK) {
            *errnode = i;
            switch (ret) {
                case E_NOKEY:
                case E_NOINDEX:
                case E_INFINDEX:
                    *p = prev;
                    break;
                default:
                    break;
            }
            *p = prev;
            *n = NULL;
            return ret;
        }
    }
    *p = prev;
    *n = current;
    return E_OK;
}

SearchPath NewSearchPath(size_t cap) { return (SearchPath){calloc(cap, sizeof(PathNode)), 0, cap}; }

void __searchPath_append(SearchPath *p, PathNode pn) {
    if (p->len >= p->cap) {
        p->cap = p->cap ? MIN(p->cap * 2, 1024) : 1;
        p->nodes = realloc(p->nodes, p->cap * sizeof(PathNode));
    }

    p->nodes[p->len++] = pn;
}

void SearchPath_AppendInfiniteIndex(SearchPath *p, int positive) {
    PathNode pn;
    pn.type = NT_INFINITE;
    pn.value.positive = positive;
    __searchPath_append(p, pn);
}

void SearchPath_AppendIndex(SearchPath *p, int idx) {
    PathNode pn;
    pn.type = NT_INDEX;
    pn.value.index = idx;
    __searchPath_append(p, pn);
}

void SearchPath_AppendKey(SearchPath *p, const char *key, const size_t len) {
    PathNode pn;
    pn.type = NT_KEY;
    pn.value.key = strndup(key, len);
    __searchPath_append(p, pn);
}

void SearchPath_AppendRoot(SearchPath *p) {
    PathNode pn;
    pn.type = NT_ROOT;
    __searchPath_append(p, pn);
}

void SearchPath_Free(SearchPath *p) {
    if (p->nodes) {
        for (int i = 0; i < p->len; i++) {
            if (p->nodes[i].type == NT_KEY) {
                free((char *)p->nodes[i].value.key);
            }
        }
    }

    free(p->nodes);
}
