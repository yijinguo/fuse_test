/* Compile with:
    gcc -Wall chat.c `pkg-config fuse3 --cflags --libs` -o chat

    =====  OPERATION: ====
    cd      : getattr + readdir
    ls      : readdir
    mkdir   : mkdir
    touch   : getattr + mknod
    cat     : getattr + read
    echo    : getattr + write
    rmdir   : rmdir
    rm      : unlink
*/

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <pthread.h>
#include<malloc.h>
#include "fileLib.h"

struct node* root;

struct node* find_node(const char *path){
	printf("find_node:%s\n", path);
	if (strcmp(path, "/") == 0) return root;
	struct node* tmp = root;
	int len = strlen(path);
	for (int l = 0, r; l < len; l = r + 1) {
		r = l;
		while(r < len && path[r] != '/') r++;
		char *node_name = malloc(sizeof(char) * (r - l + 1));
		for (int i = 0; i < r - l; i++) node_name[i] = path[l + i];
		node_name[r - l] = '\0';
		tmp = __search(tmp, node_name);
		free(node_name);
        if (!tmp) return NULL;
	}
	printf("%s\n", tmp->filename);
    return tmp;
}


static void *chat_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 1;
	root = malloc(sizeof(struct node));
	root->type = 0;
	strcpy(root->filename, "root");
	root->contents = NULL;
	root->front = root->next = root->father = root->sons = NULL;
	return NULL;
}

static int chat_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
    int res = 0;
    printf("%s: %s\n", __FUNCTION__, path);
    memset(stbuf, 0, sizeof(struct stat));

	struct node *pf = find_node(path);
 
    if (!pf) {
        res = -ENOENT;
    } else if(pf->type == 0){ //directory
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {   //file
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        if(pf->contents) stbuf->st_size = strlen(pf->contents);
        else stbuf->st_size = 0;
    }
    return res;
}

static int chat_mkdir(const char *path, mode_t mode)
{
    printf("chat_mkdir: %s\n", path);
	char dictionary[strlen(path)+1];
	strcpy(dictionary, path);
	dictionary[strlen(path)] = '\0';
	for (int i = strlen(path)-1; i > 0; i--) {
		if (path[i] == '/') break;
		dictionary[i] = '\0';
	}
    struct node *curr = find_node(dictionary);
	if (!curr || curr->type != 0) return -ENOENT;
 
    char doc_name[strlen(path) + 1];
    strcpy(doc_name, path);
    if(doc_name[strlen(doc_name) - 1] == '/') doc_name[strlen(doc_name) - 1] = '\0';
    char* name = strrchr(doc_name,'/') + 1;
    struct node* pf = __new(name, 0);
    printf("mkdir-insert: %s\n", name);
    if(__insert(curr, pf) == 1) return 0;
    else return -EEXIST;
}

static int chat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flag)
{
    (void)offset;
    (void)fi;
    puts("chat_readdir!");

    struct node* tmp = find_node(path);
    if(tmp == NULL) return -ENOENT;
    if(tmp->type == 1)return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    struct node* head = tmp->sons;
    if(head == NULL) puts("ls: This dictionary is empty.");
    while(head != NULL){
        printf("%s ",head->filename);
        filler(buf, head->filename, NULL, 0, 0);
        head = head->next;
    }
	printf("\n");
    return 0;
}


static int chat_rmdir(const char *path)
{
	puts("chat_rmdir!");
    struct node* rm = find_node(path);
    if (rm == NULL) return -ENOENT;
    if (rm->type == 1) return -ENOENT;
    if (__delete(rm->father, rm->filename, 0) == 1) return 0;
    else return -ENOENT;
}

static int chat_create(const char *path, mode_t mode,
					 	struct fuse_file_info *fi)
{
    puts("chat_create!");
    char dictionary[strlen(path)+1];
	strcpy(dictionary, path);
	dictionary[strlen(path)] = '\0';
	for (int i = strlen(path)-1; i > 0; i--) {
		if (path[i] == '/') break;
		dictionary[i] = '\0';
	}
    struct node *curr = find_node(dictionary);
    printf("%s\n", dictionary);
    //free(dictionary);
    if(curr == NULL) return -ENOENT;

    char file_name[strlen(path) + 1];
    strcpy(file_name,path);
    //if (file_name[strlen(file_name)-1] == '/') return -EISDIR;
    char* name = strrchr(file_name,'/') + 1;
    printf("%s\n", name);
	if (strlen(name) >= NAME_LENTH-1) return -EFBIG;

    struct node* new_file = __new(name, 1);
    if(__insert(curr, new_file) == 1) return 0;
    else return -EEXIST;
}

static int chat_unlink(const char *path)
{
    struct node* rm = find_node(path);
    if (rm == NULL) return -ENOENT;
    if (rm->type == 0) return -ENOENT;
    if (__delete(rm->father, rm->filename, 1) == 1) return 0;
    else return -ENOENT;
}


static int chat_write(const char *path,
                       const char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi)
{
    printf("chat_write: %s\n", path);
    struct node *target = find_node(path);
    if (target == NULL || target->type == 0) return -ENOENT;
    target->contents = malloc(sizeof(char) * (size + offset + 1));
    memcpy(target->contents + offset, buf, size);
    printf("%s\n",target->contents);
    return size;
}   
 
static int chat_read(const char *path,
                      char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    printf("chat_read: %s\n", path);
    struct node* target = find_node(path);
    if (target == NULL || target->type == 0) return -ENOENT;
    if (target->contents == NULL) return 0;

    int len = strlen(target->contents);
    printf("%d\n",len);
    if (offset < len){
        if (offset + size > len) size = len - offset;
        memcpy(buf, target->contents + offset, size);
    } else {
        size = 0;
    }
    return size;
    
}


static struct fuse_operations chat_oper = {
	.init       = chat_init,
	.getattr	= chat_getattr,
	.mkdir      = chat_mkdir,
	.rmdir  	= chat_rmdir,
	.create		= chat_create,
	.unlink 	= chat_unlink,
	.readdir	= chat_readdir,
	.write 		= chat_write,
	.read		= chat_read,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &chat_oper, NULL);
}