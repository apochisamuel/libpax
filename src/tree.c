#include "types.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Directory tree node */
typedef struct tree_node {
    char name[256];
    int entry_index;       /* -1 for virtual directories */
    int is_dir;
    struct tree_node **children;
    int child_count;
    int child_capacity;
    uint64_t subtree_size;
} tree_node;

static tree_node *create_node(const char *name, int is_dir, int idx) {
    tree_node *n = (tree_node*)calloc(1, sizeof(tree_node));
    if (!n) return NULL;
    strncpy(n->name, name, 255);
    n->name[255] = '\0';
    n->is_dir = is_dir;
    n->entry_index = idx;
    n->child_capacity = 4;
    n->children = (tree_node**)calloc(n->child_capacity, sizeof(tree_node*));
    return n;
}

static void free_tree(tree_node *n) {
    if (!n) return;
    for (int i = 0; i < n->child_count; i++) free_tree(n->children[i]);
    free(n->children);
    free(n);
}

static int add_child(tree_node *parent, tree_node *child) {
    if (parent->child_count >= parent->child_capacity) {
        int nc = parent->child_capacity * 2;
        tree_node **nch = (tree_node**)realloc(parent->children, nc * sizeof(tree_node*));
        if (!nch) return PAX_ERR_MEMORY;
        parent->children = nch;
        parent->child_capacity = nc;
    }
    parent->children[parent->child_count++] = child;
    return PAX_OK;
}

static tree_node *find_child(tree_node *parent, const char *name) {
    for (int i = 0; i < parent->child_count; i++)
        if (strcmp(parent->children[i]->name, name) == 0)
            return parent->children[i];
    return NULL;
}

tree_node *build_archive_tree(const pax_archive *ar) {
    tree_node *root = create_node("", 1, -1);
    if (!root) return NULL;

    for (int i = 0; i < ar->entry_count; i++) {
        if (!ar->entries[i].name) continue;
        char path[PAX_MAX_PATH_LEN];
        strncpy(path, ar->entries[i].name, PAX_MAX_PATH_LEN - 1);
        path[PAX_MAX_PATH_LEN - 1] = '\0';

        tree_node *current = root;
        char *saveptr = NULL;
        char *tok = strtok_r(path, "/", &saveptr);

        while (tok) {
            char *next = strtok_r(NULL, "/", &saveptr);
            tree_node *child = find_child(current, tok);
            if (!child) {
                int is_last = (next == NULL);
                int is_dir = is_last ? (ar->entries[i].entry_type == PAX_ENTRY_DIR) : 1;
                child = create_node(tok, is_dir, is_last ? i : -1);
                if (!child) { free_tree(root); return NULL; }
                add_child(current, child);
            }
            current = child;
            tok = next;
        }
    }
    return root;
}

static void compute_subtree_sizes(tree_node *n, const pax_archive *ar) {
    n->subtree_size = 0;
    if (n->entry_index >= 0 && n->entry_index < ar->entry_count)
        n->subtree_size = ar->entries[n->entry_index].size;
    for (int i = 0; i < n->child_count; i++) {
        compute_subtree_sizes(n->children[i], ar);
        n->subtree_size += n->children[i]->subtree_size;
    }
}

static int count_tree_nodes(const tree_node *n) {
    int c = 1;
    for (int i = 0; i < n->child_count; i++)
        c += count_tree_nodes(n->children[i]);
    return c;
}

static int tree_depth(const tree_node *n) {
    int max_d = 0;
    for (int i = 0; i < n->child_count; i++) {
        int d = tree_depth(n->children[i]);
        if (d > max_d) max_d = d;
    }
    return max_d + 1;
}

/* Format tree listing (ls -R style) */
static int format_tree(const tree_node *n, char *buf, size_t buf_size,
                       int depth, size_t *pos) {
    for (int i = 0; i < n->child_count; i++) {
        tree_node *c = n->children[i];
        if (*pos + depth * 2 + 256 + 20 >= buf_size) return PAX_ERR_OVERFLOW;
        for (int d = 0; d < depth; d++) { buf[(*pos)++] = ' '; buf[(*pos)++] = ' '; }
        int written = snprintf(buf + *pos, buf_size - *pos, "%s%s [%llu]\n",
                               c->name, c->is_dir ? "/" : "",
                               (unsigned long long)c->subtree_size);
        if (written > 0) *pos += written;
        if (c->is_dir) format_tree(c, buf, buf_size, depth + 1, pos);
    }
    return PAX_OK;
}

int print_archive_tree(const pax_archive *ar, char *buf, size_t buf_size) {
    tree_node *root = build_archive_tree(ar);
    if (!root) return PAX_ERR_MEMORY;
    compute_subtree_sizes(root, ar);
    size_t pos = 0;
    int rc = format_tree(root, buf, buf_size, 0, &pos);
    if (pos < buf_size) buf[pos] = '\0';
    free_tree(root);
    return rc;
}

/* Count files at each depth level */
void depth_distribution(const pax_archive *ar, int *dist, int max_depth) {
    memset(dist, 0, max_depth * sizeof(int));
    for (int i = 0; i < ar->entry_count; i++) {
        if (!ar->entries[i].name) continue;
        int d = path_depth(ar->entries[i].name);
        if (d < max_depth) dist[d]++;
    }
}

/* Find largest directory by total content size */
int find_largest_dir(const pax_archive *ar) {
    tree_node *root = build_archive_tree(ar);
    if (!root) return -1;
    compute_subtree_sizes(root, ar);
    
    int best_idx = -1;
    uint64_t best_size = 0;
    for (int i = 0; i < root->child_count; i++) {
        if (root->children[i]->is_dir && root->children[i]->subtree_size > best_size) {
            best_size = root->children[i]->subtree_size;
            best_idx = root->children[i]->entry_index;
        }
    }
    free_tree(root);
    return best_idx;
}
