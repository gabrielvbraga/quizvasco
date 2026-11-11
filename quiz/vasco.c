#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#define VERSION "1.0.0"
#define MAX_PLAYERS 100
#define MAX_NAME_LENGTH 50
#define RANKING_FILE "ranking.txt"
#define RANKING_SHOWN 10
#define NUM_QUESTIONS 8
#define NUM_OPTIONS 4

typedef struct {
  char nome[MAX_NAME_LENGTH];
  int pontuacao;
} Jogador;

typedef struct {
  const char *pergunta;
  const char *opcoes[NUM_OPTIONS];
  int respostaCorreta; /* 1 a 4 */
} Pergunta;

static const Pergunta perguntas[NUM_QUESTIONS] = {
    {"Qual o principal rival do Vasco?",
     {"Botafogo", "Fluminense", "Flamengo", "Corinthians"},
     3},
    {"Em que ano o Clube de Regatas Vasco da Gama foi fundado?",
     {"1890", "1898", "1897", "1902"},
     2},
    {"Quantas vezes o Vasco foi campeao do Campeonato Brasileiro?",
     {"7", "5", "3", "4"},
     4},
    {"Qual e o atual presidente (2024) do Vasco?",
     {"Dinamite", "Eurico Miranda", "Jorge Salgado", "Pedrinho"},
     4},
    {"Quantas vezes o Vasco ganhou o Campeonato Carioca?",
     {"12", "25", "18", "24"},
     4},
    {"Qual o nome do jogador da base do Vasco que foi vendido para a "
     "Internazionale da Italia?",
     {"Roberto Dinamite", "Ze Gabriel", "Philippe Coutinho", "Douglas Luiz"},
     3},
    {"Quem criou o primeiro hino do Vasco, o HINO TRIUNFAL VASCO DA GAMA?",
     {"Joao de Freitas", "Joaquim Barros", "Lamartine Babo", "Ernani Correa"},
     2},
    {"Por que foi dado o nome para o clube de Vasco?",
     {"Porque foram influenciados por Portugal",
      "Porque o fundador do time se chamava Vasco",
      "Porque eles acharam bonito o nome", "Pelo navegador Vasco da Gama"},
     4}};

static Jogador ranking[MAX_PLAYERS + 1];
static int numJogadores = 0;
static int usarCores = 1;

/* ---------- terminal ---------- */

static void configurarTerminal(void) {
#ifdef _WIN32
  HANDLE saida = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD modo = 0;
  usarCores = GetConsoleMode(saida, &modo) &&
              SetConsoleMode(saida, modo | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

static const char *cor(const char *codigo) { return usarCores ? codigo : ""; }

#define VERMELHO cor("\033[1;31m")
#define VERDE cor("\033[1;32m")
#define BRANCO cor("\033[1;37m")
#define RESET cor("\033[0m")

static void limparTela(void) {
#ifdef _WIN32
  if (!usarCores) { /* console antigo, sem suporte a codigos ANSI */
    system("cls");
    return;
  }
#endif
  printf("\033[2J\033[H");
  fflush(stdout);
}

/* ---------- entrada ---------- */

/* Le uma linha inteira, sem o \n. Retorna 0 se a entrada acabou (EOF). */
static int lerLinha(char *buffer, size_t tamanho) {
  if (fgets(buffer, (int)tamanho, stdin) == NULL) {
    return 0;
  }
  size_t fim = strcspn(buffer, "\r\n");
  if (buffer[fim] == '\0' && fim == tamanho - 1) {
    int c; /* linha maior que o buffer: descarta o resto */
    while ((c = getchar()) != '\n' && c != EOF)
      ;
  }
  buffer[fim] = '\0';
  return 1;
}

/* Le um inteiro. Retorna 1 = ok, 0 = texto invalido, -1 = EOF. */
static int lerInteiro(int *valor) {
  char linha[32];
  char *resto;
  if (!lerLinha(linha, sizeof linha)) {
    return -1;
  }
  long n = strtol(linha, &resto, 10);
  if (resto == linha) {
    return 0;
  }
  while (*resto == ' ' || *resto == '\t') {
    resto++;
  }
  if (*resto != '\0' || n < -1000 || n > 1000) {
    return 0;
  }
  *valor = (int)n;
  return 1;
}

static void pausar(void) {
  char linha[8];
  printf("\nPressione Enter para continuar...");
  fflush(stdout);
  lerLinha(linha, sizeof linha);
}

/* Remove espacos do inicio e do fim. */
static void apararEspacos(char *texto) {
  size_t inicio = strspn(texto, " \t");
  size_t tamanho = strlen(texto + inicio);
  memmove(texto, texto + inicio, tamanho + 1);
  while (tamanho > 0 && (texto[tamanho - 1] == ' ' || texto[tamanho - 1] == '\t')) {
    texto[--tamanho] = '\0';
  }
}

/* ---------- ranking ---------- */

/* Maior pontuacao primeiro; em caso de empate, quem jogou antes fica na frente. */
static void ordenarRanking(void) {
  for (int i = 1; i < numJogadores; i++) {
    Jogador atual = ranking[i];
    int j = i - 1;
    while (j >= 0 && ranking[j].pontuacao < atual.pontuacao) {
      ranking[j + 1] = ranking[j];
      j--;
    }
    ranking[j + 1] = atual;
  }
}

/* Cada linha do arquivo e "nome pontuacao"; o nome pode ter espacos. */
static void carregarRanking(void) {
  char linha[128];
  FILE *arquivo = fopen(RANKING_FILE, "r");
  if (arquivo == NULL) {
    return;
  }
  while (numJogadores < MAX_PLAYERS && fgets(linha, sizeof linha, arquivo)) {
    linha[strcspn(linha, "\r\n")] = '\0';
    apararEspacos(linha);
    char *separador = strrchr(linha, ' ');
    char *resto;
    if (separador == NULL) {
      continue;
    }
    long pontos = strtol(separador + 1, &resto, 10);
    if (resto == separador + 1 || *resto != '\0' || pontos < 0 ||
        pontos > NUM_QUESTIONS) {
      continue;
    }
    *separador = '\0';
    apararEspacos(linha);
    if (linha[0] == '\0') {
      continue;
    }
    snprintf(ranking[numJogadores].nome, MAX_NAME_LENGTH, "%.*s",
             MAX_NAME_LENGTH - 1, linha);
    ranking[numJogadores].pontuacao = (int)pontos;
    numJogadores++;
  }
  fclose(arquivo);
  ordenarRanking();
}

static void salvarRanking(void) {
  FILE *arquivo = fopen(RANKING_FILE, "w");
  if (arquivo == NULL) {
    printf("\nNao foi possivel salvar o ranking em %s.\n", RANKING_FILE);
    return;
  }
  for (int i = 0; i < numJogadores; i++) {
    fprintf(arquivo, "%s %d\n", ranking[i].nome, ranking[i].pontuacao);
  }
  fclose(arquivo);
}

static void registrarPontuacao(const char *nome, int pontuacao) {
  snprintf(ranking[numJogadores].nome, MAX_NAME_LENGTH, "%s", nome);
  ranking[numJogadores].pontuacao = pontuacao;
  numJogadores++;
  ordenarRanking();
  if (numJogadores > MAX_PLAYERS) {
    numJogadores = MAX_PLAYERS; /* descarta o ultimo colocado */
  }
  salvarRanking();
}

/* ---------- telas ---------- */

/* Retorna 0 se a entrada acabou no meio do quiz. */
static int jogar(void) {
  char nome[MAX_NAME_LENGTH];
  int ordem[NUM_QUESTIONS];
  int pontuacao = 0;

  limparTela();
  printf("Digite seu nome: ");
  fflush(stdout);
  if (!lerLinha(nome, sizeof nome)) {
    return 0;
  }
  apararEspacos(nome);
  if (nome[0] == '\0') {
    strcpy(nome, "Anonimo");
  }

  /* Embaralha a ordem das perguntas (Fisher-Yates). */
  for (int i = 0; i < NUM_QUESTIONS; i++) {
    ordem[i] = i;
  }
  for (int i = NUM_QUESTIONS - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    int temp = ordem[i];
    ordem[i] = ordem[j];
    ordem[j] = temp;
  }

  limparTela();
  printf("\n%s--- Inicio do Quiz ---%s\n", BRANCO, RESET);

  for (int i = 0; i < NUM_QUESTIONS; i++) {
    const Pergunta *p = &perguntas[ordem[i]];
    int resposta = 0;

    printf("\nPergunta %d de %d: %s\n", i + 1, NUM_QUESTIONS, p->pergunta);
    for (int j = 0; j < NUM_OPTIONS; j++) {
      printf("%d. %s\n", j + 1, p->opcoes[j]);
    }

    for (;;) {
      printf("Sua resposta: ");
      fflush(stdout);
      int lido = lerInteiro(&resposta);
      if (lido == -1) {
        return 0;
      }
      if (lido == 1 && resposta >= 1 && resposta <= NUM_OPTIONS) {
        break;
      }
      printf("\nResposta invalida! Por favor, escolha um numero entre 1 e %d.\n",
             NUM_OPTIONS);
    }

    if (resposta == p->respostaCorreta) {
      pontuacao++;
      printf("\n%sCorreto!%s\n", VERDE, RESET);
    } else {
      printf("\n%sErrado!%s A resposta correta e %s.\n", VERMELHO, RESET,
             p->opcoes[p->respostaCorreta - 1]);
    }
  }

  printf("\nQuiz finalizado! %sSua pontuacao: %d/%d%s\n", BRANCO, pontuacao,
         NUM_QUESTIONS, RESET);
  registrarPontuacao(nome, pontuacao);
  pausar();
  return 1;
}

static void mostrarRegras(void) {
  limparTela();
  printf("\n--- Regras do Quiz ---\n");
  printf("1. O quiz consiste em %d perguntas.\n", NUM_QUESTIONS);
  printf("2. Cada pergunta tem %d opcoes de resposta, mas apenas uma esta "
         "correta.\n",
         NUM_OPTIONS);
  printf("3. Para cada resposta correta, voce ganha 1 ponto.\n");
  printf("4. Boa sorte!\n");
  pausar();
}

static void mostrarRanking(void) {
  limparTela();
  printf("\n--- Ranking ---\n");
  if (numJogadores == 0) {
    printf("Ninguem jogou ainda.\n");
  }
  for (int i = 0; i < numJogadores && i < RANKING_SHOWN; i++) {
    printf("%d. %s - %d pontos\n", i + 1, ranking[i].nome,
           ranking[i].pontuacao);
  }
  pausar();
}

static void mostrarCreditos(void) {
  limparTela();
  printf("\n--- Creditos ---\n");
  printf("Desenvolvido por: gabriel\n");
  printf("Versao: %s\n", VERSION);
  pausar();
}

int main(void) {
  int opcao = 0;

  configurarTerminal();
  srand((unsigned)time(NULL));
  carregarRanking();

  while (opcao != 5) {
    limparTela();
    printf("\n%s--- QUIZ VASCO DA GAMA ---%s\n", VERMELHO, RESET);
    printf("1. Iniciar Quiz\n");
    printf("2. Regras do Quiz\n");
    printf("3. Exibir Ranking\n");
    printf("4. Creditos\n");
    printf("5. Sair\n");
    printf("Escolha uma opcao: ");
    fflush(stdout);

    int lido = lerInteiro(&opcao);
    if (lido == -1) {
      break;
    }
    if (lido == 0) {
      opcao = 0;
    }

    if (opcao == 1) {
      if (!jogar()) {
        break;
      }
    } else if (opcao == 2) {
      mostrarRegras();
    } else if (opcao == 3) {
      mostrarRanking();
    } else if (opcao == 4) {
      mostrarCreditos();
    } else if (opcao != 5) {
      printf("Opcao invalida! Tente novamente.\n");
      pausar();
    }
  }

  printf("\nSaindo do quiz...\n");
  return 0;
}
