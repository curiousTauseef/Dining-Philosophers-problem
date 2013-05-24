#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/errno.h>

#define TABLE 5
#define DELAY 50000
#define DISHES 5

typedef struct node {
    struct node *next;
    pthread_cond_t sleep;
} node;

typedef struct {
    node *head, *tail;
    pthread_mutex_t *mut;
    int runmax;
    int size;
} fifo;

fifo *fifoInit(int runmax);
void fifoDelete(fifo *q);
void fifoActivate(fifo *q, pthread_mutex_t *mut, int id);
void fifoInsert(fifo *q, int id);
void *philosopher(void *id);
pthread_mutex_t forks[TABLE];
fifo *queue;


// Προς υλοποίηση
int main(int argc, char **argv) {
    pthread_t table[TABLE];
    int i;

    queue = fifoInit(TABLE/2);

    for (i = 0; i < TABLE; i++) {
        pthread_mutex_init(&(forks[i]), NULL);
    }

    for (i = 0; i < TABLE; i++) {
        pthread_create(&table[i], NULL, philosopher, (void *) i);
    }
    for (i = 0; i < TABLE; i++) {
        pthread_join(table[i], NULL);
    }

    fifoDelete(queue);
    return 0;
}

// Προς υλοποίηση
void *philosopher(void *num) {
    int id;
    int i;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    id = (int) num;
    pthread_mutex_lock(&mutex);
    for (i = 0; i < 2; i++) {
        fifoActivate(queue, &mutex, id);
        usleep(DELAY);
        fifoInsert(queue, id);
    }
    for (i = 0; i < DISHES; i++) {
        printf("P-%d Start\n", id);
        while (1) {
            fifoActivate(queue, &mutex, id);
            if (pthread_mutex_trylock (&(forks[(id + 1) % TABLE])) == EBUSY) {
                fifoInsert(queue, id);
                continue;
            }
            printf("%d gotfork%d\n", id, (id + 1)%TABLE);
            usleep(DELAY);
            if (pthread_mutex_trylock(&(forks[id])) == EBUSY) {
                pthread_mutex_unlock(&(forks[(id + 1)%TABLE]));
                printf("%d leave fork%d\n", id, (id + 1) % TABLE);
                fifoInsert(queue, id);
                continue;
            }
            printf("%d got fork%d and eats\n", id, id);
            usleep(DELAY);
            pthread_mutex_unlock(&(forks[id]));
            pthread_mutex_unlock(&(forks[(id + 1) % TABLE]));
            fifoInsert(queue, id);
            break;
        }
        printf("Stoped: %d\n", id);
    }
    printf("Stoped: %d\n", id);
    pthread_mutex_unlock(&mutex);
}

fifo *fifoInit(int runmax) {
    fifo *q;
    q = (fifo *) malloc(sizeof (fifo));
    if (q == NULL)
        return (NULL);
    q->mut = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    if (q->mut == NULL) {
        free(q);
        return (NULL);
    }
    pthread_mutex_init(q->mut, NULL);

    q->runmax = runmax;
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;

    return (q);
}

void fifoDelete(fifo *q) {
    if (q->head != NULL) {
        printf("fifoDelete\n");
        exit(1);
    }
    pthread_mutex_destroy(q->mut);
    free(q->mut);
    free(q);
    return;
}

// Προς υλοποίηση
void fifoActivate(fifo *q, pthread_mutex_t *mut, int id) {
    node *new;
    pthread_mutex_lock(q->mut);
    q->size += 1;
    if ((q->size) > (q->runmax)) {
        new = (node *) malloc(sizeof (node));
        if (new == NULL) {
            exit(1);
        }
        pthread_cond_init(&(new->sleep), NULL);
        new->next = NULL;
        if (q->tail == NULL) {
            q->head = q->tail = new;
        } else {
            q->tail->next = new;
            q->tail = new;
        }
        pthread_mutex_unlock(q->mut);
        pthread_cond_wait(&(new->sleep), mut);
    } else {
        pthread_mutex_unlock(q->mut);
    }
}

// Προς υλοποίηση
void fifoInsert(fifo *q, int id) {
    node *last;
    pthread_mutex_lock(q->mut);
    q->size -= 1;
    if (q->head != NULL) {
        last = q->head;
        q->head = last->next;
        if (q->head == NULL) {
            q->tail = NULL;
        }
        last->next = NULL;
        pthread_cond_signal(&(last->sleep));
    }
    pthread_mutex_unlock(q->mut);
}