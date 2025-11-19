#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <locale.h>
#include <termios.h>
#include <unistd.h>

// Definições de constantes para otimização
#define MAX_NOME 50
#define INCREMENTO_CAPACIDADE 10 
#define WIDTH 60

typedef struct {
    char nome[MAX_NOME];
    int duracao;
    int prioridade;
} Curso;

typedef struct {
    char nome[MAX_NOME];
    int id;
} Aluno;

typedef struct {
    Aluno aluno;
    int curso_index;
    time_t timestamp;
} Inscricao;

typedef struct NoFila {
    Inscricao inscricao;
    struct NoFila* proximo;
} NoFila;

typedef struct {
    NoFila* frente;
    NoFila* tras;
    int tamanho;
} Fila;

typedef struct NoPilha {
    char acao[MAX_NOME];
    struct NoPilha* proximo;
} NoPilha;

typedef struct {
    NoPilha* topo;
    int tamanho;
} Pilha;

Curso* cursos = NULL;
int num_cursos = 0;
int capacidade_cursos = 0;

Aluno* alunos = NULL;
int num_alunos = 0;
int capacidade_alunos = 0;

int visual_width(const char *s) {
    int width = 0;
    wchar_t wc;
    mbstate_t state;
    memset(&state, 0, sizeof state);
    const char *p = s;

    while (*p) {
        size_t len = mbrtowc(&wc, p, MB_CUR_MAX, &state);
        if (len == (size_t)-1 || len == (size_t)-2) break;
        int w = wcwidth(wc);
        if (w > 0) width += w;
        p += len;
    }
    return width;
}

void print_border_top() {
    printf("╔");
    for (int i = 0; i < WIDTH - 2; i++) printf("═");
    printf("╗\n");
}

void print_border_bottom() {
    printf("╚");
    for (int i = 0; i < WIDTH - 2; i++) printf("═");
    printf("╝\n");
}

void print_line(const char *text) {
    printf("║");
    int content_width = visual_width(text);
    int padding = WIDTH - 2 - content_width;
    printf("%s", text);
    for (int j = 0; j < padding; j++) printf(" ");
    printf("║\n");
}

void limparBufferEntrada() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int capturaTecla() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    if (ch == 27) {
        getchar();
        ch = getchar();
        if (ch == 'A') ch = 'w';
        else if (ch == 'B') ch = 's';
        else ch = 0;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void esperarPressionarQ() {
    char input[10];
    print_border_top();
    print_line("Pressione 'q' para voltar ao menu...");
    print_border_bottom();
    do {
        printf("> ");
        if (fgets(input, sizeof(input), stdin)) {
            if (input[0] == 'q' || input[0] == 'Q') break;
        }
    } while (1);
}

void inicializarFila(Fila* f) {
    f->frente = NULL;
    f->tras = NULL;
    f->tamanho = 0;
}

int filaVazia(Fila* f) {
    return f->tamanho == 0;
}

void enfileirar(Fila* f, Inscricao inscricao) {
    NoFila* novo = (NoFila*)malloc(sizeof(NoFila));
    if (!novo) {
        printf("Erro: Falha na alocação de memória.\n");
        return;
    }
    novo->inscricao = inscricao;
    novo->proximo = NULL;
    if (filaVazia(f)) {
        f->frente = f->tras = novo;
    } else {
        f->tras->proximo = novo;
        f->tras = novo;
    }
    f->tamanho++;
}

Inscricao desenfileirar(Fila* f) {
    if (filaVazia(f)) {
        printf("Erro: Fila vazia.\n");
        exit(1);
    }
    NoFila* temp = f->frente;
    Inscricao inscricao = temp->inscricao;
    f->frente = f->frente->proximo;
    if (f->frente == NULL) f->tras = NULL;
    free(temp);
    f->tamanho--;
    return inscricao;
}

void inicializarPilha(Pilha* p) {
    p->topo = NULL;
    p->tamanho = 0;
}

int pilhaVazia(Pilha* p) {
    return p->tamanho == 0;
}

void empilhar(Pilha* p, const char* acao) {
    NoPilha* novo = (NoPilha*)malloc(sizeof(NoPilha));
    if (!novo) {
        printf("Erro: Falha na alocação de memória.\n");
        return;
    }
    strncpy(novo->acao, acao, MAX_NOME-1);
    novo->acao[MAX_NOME-1] = '\0';
    novo->proximo = p->topo;
    p->topo = novo;
    p->tamanho++;
}

char* desempilhar(Pilha* p) {
    if (pilhaVazia(p)) {
        return NULL;
    }
    NoPilha* temp = p->topo;
    char* acao = (char*)malloc(strlen(temp->acao) + 1);
    strcpy(acao, temp->acao);
    p->topo = p->topo->proximo;
    free(temp);
    p->tamanho--;
    return acao;
}

void trocar(Curso* a, Curso* b) {
    Curso temp = *a;
    *a = *b;
    *b = temp;
}

int particionar(Curso arr[], int baixo, int alto) {
    int pivo = arr[alto].prioridade;
    int i = baixo - 1;
    for (int j = baixo; j < alto; j++) {
        if (arr[j].prioridade >= pivo) {
            i++;
            trocar(&arr[i], &arr[j]);
        }
    }
    trocar(&arr[i + 1], &arr[alto]);
    return i + 1;
}

void quickSort(Curso arr[], int baixo, int alto) {
    if (baixo < alto) {
        int pi = particionar(arr, baixo, alto);
        quickSort(arr, baixo, pi - 1);
        quickSort(arr, pi + 1, alto);
    }
}

int buscaBinariaCurso(Curso arr[], int n, int prioridade) {
    int baixo = 0, alto = n - 1;
    while (baixo <= alto) {
        int meio = baixo + (alto - baixo) / 2;
        if (arr[meio].prioridade == prioridade) return meio;
        if (arr[meio].prioridade > prioridade) baixo = meio + 1;
        else alto = meio - 1;
    }
    return -1;
}

int buscaLinearAluno(Aluno arr[], int n, int id) {
    for (int i = 0; i < n; i++) {
        if (arr[i].id == id) return i;
    }
    return -1;
}

void expandirCursos() {
    capacidade_cursos += INCREMENTO_CAPACIDADE;
    cursos = (Curso*)realloc(cursos, capacidade_cursos * sizeof(Curso));
    if (!cursos) {
        printf("Erro: Falha na realocação de memória para cursos.\n");
        exit(1);
    }
}

void expandirAlunos() {
    capacidade_alunos += INCREMENTO_CAPACIDADE;
    alunos = (Aluno*)realloc(alunos, capacidade_alunos * sizeof(Aluno));
    if (!alunos) {
        printf("Erro: Falha na realocação de memória para alunos.\n");
        exit(1);
    }
}

void carregarCursos() {
    FILE* arquivo = fopen("cursos.txt", "r");
    if (!arquivo) {
        printf("Arquivo cursos.txt não encontrado. Criando novo.\n");
        return;
    }
    char linha[256];
    while (fgets(linha, sizeof(linha), arquivo)) {
        if (num_cursos >= capacidade_cursos) expandirCursos();
        sscanf(linha, "%[^,],%d,%d", cursos[num_cursos].nome, &cursos[num_cursos].duracao, &cursos[num_cursos].prioridade);
        num_cursos++;
    }
    fclose(arquivo);
}

void salvarCursos() {
    FILE* arquivo = fopen("cursos.txt", "w");
    if (!arquivo) {
        printf("Erro ao abrir cursos.txt para escrita.\n");
        return;
    }
    for (int i = 0; i < num_cursos; i++) {
        fprintf(arquivo, "%s,%d,%d\n", cursos[i].nome, cursos[i].duracao, cursos[i].prioridade);
    }
    fclose(arquivo);
}

void carregarAlunos() {
    FILE* arquivo = fopen("alunos.txt", "r");
    if (!arquivo) {
        printf("Arquivo alunos.txt não encontrado. Criando novo.\n");
        return;
    }
    char linha[256];
    while (fgets(linha, sizeof(linha), arquivo)) {
        if (num_alunos >= capacidade_alunos) expandirAlunos();
        sscanf(linha, "%[^,],%d", alunos[num_alunos].nome, &alunos[num_alunos].id);
        num_alunos++;
    }
    fclose(arquivo);
}

void salvarAlunos() {
    FILE* arquivo = fopen("alunos.txt", "w");
    if (!arquivo) {
        printf("Erro ao abrir alunos.txt para escrita.\n");
        return;
    }
    for (int i = 0; i < num_alunos; i++) {
        fprintf(arquivo, "%s,%d\n", alunos[i].nome, alunos[i].id);
    }
    fclose(arquivo);
}

void adicionarCurso() {
    if (num_cursos >= capacidade_cursos) expandirCursos();
    printf("Nome do curso: ");
    scanf(" %[^\n]", cursos[num_cursos].nome);
    printf("Duração (horas): ");
    scanf("%d", &cursos[num_cursos].duracao);
    printf("Prioridade (1-10): ");
    scanf("%d", &cursos[num_cursos].prioridade);
    num_cursos++;
    printf("Curso adicionado.\n");
}

void adicionarAluno() {
    if (num_alunos >= capacidade_alunos) expandirAlunos();
    printf("Nome do aluno: ");
    scanf(" %[^\n]", alunos[num_alunos].nome);
    printf("ID do aluno: ");
    scanf("%d", &alunos[num_alunos].id);
    num_alunos++;
    printf("Aluno adicionado.\n");
}
void inscreverAluno(Fila* fila, Pilha* historico) {
    int id_aluno, index_curso;
    printf("ID do aluno: ");
    scanf("%d", &id_aluno);
    int aluno_index = buscaLinearAluno(alunos, num_alunos, id_aluno);
    if (aluno_index == -1) {
        printf("Erro: Aluno não encontrado.\n");
        return;
    }
    printf("Nome do curso: ");
    char nome_curso[MAX_NOME];
    scanf(" %[^\n]", nome_curso);
    for (index_curso = 0; index_curso < num_cursos; index_curso++) {
        if (strcmp(cursos[index_curso].nome, nome_curso) == 0) break;
    }
    if (index_curso == num_cursos) {
        printf("Erro: Curso não encontrado.\n");
        return;
    }
    Inscricao nova;
    nova.aluno = alunos[aluno_index];
    nova.curso_index = index_curso;
    nova.timestamp = time(NULL);
    enfileirar(fila, nova);
    char acao[100];
    snprintf(acao, sizeof(acao), "Inscrição: %s em %s", alunos[aluno_index].nome, cursos[index_curso].nome);
    empilhar(historico, acao);
    printf("Inscrição realizada.\n");
}

void processarInscricao(Fila* fila, Pilha* historico) {
    if (filaVazia(fila)) {
        printf("Nenhuma inscrição para processar.\n");
        return;
    }
    Inscricao processada = desenfileirar(fila);
    printf("Processando inscrição: %s em %s\n", processada.aluno.nome, cursos[processada.curso_index].nome);
    char acao[100];
    snprintf(acao, sizeof(acao), "Processamento: %s em %s", processada.aluno.nome, cursos[processada.curso_index].nome);
    empilhar(historico, acao);
}

void ordenarCursos() {
    if (num_cursos > 0)
        quickSort(cursos, 0, num_cursos - 1);
    printf("Cursos ordenados por prioridade.\n");
}

void buscarCurso() {
    int prioridade;
    printf("Prioridade do curso: ");
    scanf("%d", &prioridade);
    int index = buscaBinariaCurso(cursos, num_cursos, prioridade);
    if (index != -1) {
        printf("Curso encontrado: %s, Duração: %d horas\n", cursos[index].nome, cursos[index].duracao);
    } else {
        printf("Curso não encontrado.\n");
    }
}

void undo(Pilha* historico) {
    char* acao = desempilhar(historico);
    if (acao) {
        printf("Undo: %s\n", acao);
        free(acao);
    } else {
        printf("Nenhuma ação para desfazer.\n");
    }
}

int main() {
    setlocale(LC_ALL, "");

    carregarCursos();
    carregarAlunos();

    Fila fila_inscricoes;
    Pilha historico;
    inicializarFila(&fila_inscricoes);
    inicializarPilha(&historico);

    const char* opcoes[] = {
        "Adicionar Curso",
        "Adicionar Aluno",
        "Inscrever Aluno em Curso",
        "Processar Inscrição",
        "Ordenar Cursos por Prioridade",
        "Buscar Curso por Prioridade",
        "Undo Última Ação",
        "Sair"
    };
    int total_opcoes = 8;
    int selected = 0;
    int running = 1;

    while (running) {
        system("clear");
        print_border_top();
        print_line("      <=== SISTEMA DE GERENCIAMENTO DE CURSOS ===>");
        print_line("");
        for (int i = 0; i < total_opcoes; i++) {
            char buffer[100];
            if (i == selected)
                snprintf(buffer, sizeof(buffer), " < %s >", opcoes[i]);
            else
                snprintf(buffer, sizeof(buffer), "    %s   ", opcoes[i]);
            print_line(buffer);
        }
        print_line("");
        print_line("Use as setas ou W/S para navegar, Enter para selecionar.");
        print_border_bottom();

        int tecla = capturaTecla();

        if (tecla == 'w') {
            selected = (selected - 1 + total_opcoes) % total_opcoes;
        } else if (tecla == 's') {
            selected = (selected + 1) % total_opcoes;
        } else if (tecla == '\n' || tecla == '\r') {
            switch (selected) {
                case 0:
                    adicionarCurso();
                    esperarPressionarQ();
                    break;
                case 1:
                    adicionarAluno();
                    esperarPressionarQ();
                    break;
                case 2:
                    inscreverAluno(&fila_inscricoes, &historico);
                    esperarPressionarQ();
                    break;
                case 3:
                    processarInscricao(&fila_inscricoes, &historico);
                    esperarPressionarQ();
                    break;
                case 4:
                    ordenarCursos();
                    esperarPressionarQ();
                    break;
                case 5:
                    buscarCurso();
                    esperarPressionarQ();
                    break;
                case 6:
                    undo(&historico);
                    esperarPressionarQ();
                    break;
                case 7:
                    salvarCursos();
                    salvarAlunos();

                    while (fila_inscricoes.frente != NULL) {
                        NoFila* tmp = fila_inscricoes.frente;
                        fila_inscricoes.frente = tmp->proximo;
                        free(tmp);
                    }
                    fila_inscricoes.tras = NULL;
                    fila_inscricoes.tamanho = 0;

                    while (historico.topo != NULL) {
                        NoPilha* tmp = historico.topo;
                        historico.topo = tmp->proximo;
                        free(tmp);
                    }
                    historico.tamanho = 0;

                    if (cursos) {
                        free(cursos);
                        cursos = NULL;
                        num_cursos = 0;
                        capacidade_cursos = 0;
                    }
                    if (alunos) {
                        free(alunos);
                        alunos = NULL;
                        num_alunos = 0;
                        capacidade_alunos = 0;
                    }

                    printf("Saindo... Dados salvos.\n");
                    running = 0;
                    break;
            }
        }
    }

    return 0;
}

