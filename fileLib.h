#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#define NAME_LENTH 256

static struct node {
    int type; //1:file; 0:dictionary
    char filename[NAME_LENTH];
    char *contents;
	struct node* front;
	struct node* next;
    struct node* sons;
    struct node* father;
};

struct node* __search(struct node* curr, const char *name){
    if (strcmp(name, "") == 0) return curr;
    struct node* tmp = curr->sons;
    if (tmp == NULL) return NULL;
    while (tmp != NULL) {
        if (strcmp(tmp->filename, name) < 0) tmp = tmp->next;
        else if (strcmp(tmp->filename, name) == 0) return tmp;
        else break;
    }
    return NULL;
}

struct node* __new(const char* name, int type){
	struct node* new_node = malloc (sizeof(struct node));
    new_node->type = type;
	strcpy(new_node->filename, name);
    if (type == 1) new_node->contents = "";
    else new_node->contents = NULL;
	new_node->front = new_node->next = new_node->sons = new_node->father = NULL;
	return new_node;
}

int __insert(struct node* curr, struct node* pf){
    printf("__insert %s to %s\n", pf->filename, curr->filename);
    struct node *tmp = curr->sons, *last;
    pf->father = curr;
    if (tmp == NULL) {
        curr->sons = pf;
        return 1;
    }
    while (tmp != NULL) {
        if (strcmp(tmp->filename, pf->filename) < 0) {
            last = tmp;
            tmp = tmp->next;
        } else if (strcmp(tmp->filename, pf->filename) == 0 && last->type == pf->type) {
            free(pf);
            return 0;
        } else {
            break;
        }
    }
    last->next = pf;
    pf->front = last;
    pf->next = tmp;
    if (tmp != NULL) tmp->front = pf;
    return 1;
}

int __delete(struct node* curr, const char* name, int type){
    struct node *tmp = curr->sons;
    if (tmp == NULL) return 0;

    while (tmp != NULL) {
        if (strcmp(tmp->filename, name) < 0) tmp = tmp->next;
        else if (strcmp(tmp->filename, name) == 0 && tmp->type == type) break;
        else return 0;
    }
    if (tmp == NULL) return 0;

    struct node *l = tmp->front, *r = tmp->next;
    if (l != NULL) l->next = r;
    if (r != NULL) r->front = l;
    if (tmp == curr->sons) curr->sons = r;
    free(tmp);
    return 1;
}
