#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#define MAXLINHA 1123
#define MAXARQUIVOS 1000

using namespace std;

typedef long long int Offset;
typedef unsigned long long int Hash;

//indices para o vetor de indices (serao inseridos na arvore)
typedef struct index_t{
    Hash hash;
    long long int offset;

    index_t(long long int _h, unsigned long int _o) : hash(_h), offset(_o) {}
} index_t;

//offsets. O prox e pra quando se tem mais de um hash igual
typedef struct offsets_t{    //lista encadeada dos offsets
    int indexArquivo;
    Offset offset;
    struct offsets_t *prox;
} offsets_t;

typedef struct nodo_t{
    nodo_t **filhos, *prox, *pai;
    offsets_t **offsets;
    Hash *keys;
    int quantidadeKeys, quantidadeFilhos;
    bool folha;

    /*~nodo_t() { //deconstrutor
        free(keys);
        free(filhos);
    }*/
}nodo_t;

typedef vector<index_t> vind;

//Inicialização e funçõe auxiliares
//Cria a hash a partir de uma string. Função que foi pega na internet.
Hash hashFunction(char *str);
//Função que recebe o nome de arquivo a ser aberto e retorna o ponteiro para o arquivo. A função já trata dos possiveis erros que podem acontecer na abertura de um arquivo.
FILE* abrirArquivo(char arquivoEntrada[]);
//Função para comparar duas variáves de tipo index_t. Foi criada pra dar suporte a função de sort da biblioteca algorithm.
bool compareIndex(const index_t &_a, const index_t &_b);
//Função que le o arquivo .csv e guarda as hashs e os offsets no vector de index_t
void leituraArquivo(vind &indices, FILE *entrada);

//BulkLoading
//Função principal do Bulk Loading. Retorna se nenhum erro aconteceu.
int bulk_loading(nodo_t* &arvore, vind &indices, int ordem);
//Função que liga o filho recém criado de Bulk Loading com o pai. Se não tem espaço no pai, a função também lida com isso criando novos nós.
int checaPai(nodo_t *filhoAtual, nodo_t** pAtual, Hash hashQueVem, int ordem);
//Função que cria e inicaliza nodo e retorna o ponteiro.
nodo_t* criaNodo(int ordem, bool folha);
//Função que cria e inicializa offset e retorna o ponteiro pro offset. Ela recebe o offset a ser inserido e o antigo offset de um hash, para poder inserir o novo offset no começo da lista, e não no final.
offsets_t* criaOffset(Offset o, offsets_t *p);

//Destruir a árvore
//Função para destruir a arvore (desalocar da memória)
void mataArvore(nodo_t *n);
//Função para destruir os offsets (desalocá-los da memória)
void mataOffsets(offsets_t *o);

//Imprimir a árvore
//Função que cria o arquivo .dot, transforma-o em .png e abre a imagem da arvore.
int imprimeArvore(nodo_t *arvore);
//Função auxiliar da imprimeArvore que imprime os nodos.
void imprimeNodos(FILE *dotFile, nodo_t *n, int *numeroNodo, int liga);
//Função que remove o Ultimo elemento de um nó e ajeita o nó de acordo com o resultado.
void removeUltimo(nodo_t *paiAtual, int ordem);
//Função que imprime a tupla, dado um no e uma posição
void imprimeTupla(nodo_t *nodoAtual, int indiceElemento);

//Buscar Elemento
//Função que retorna o endereço do nó em que se encontra o elemento "procurando" e coloca a posição do elemento na variável indice. A função retorna NULL caso o elemento não seja encontrado
nodo_t *achaElemento(nodo_t* noAtual, int &indice, Hash procurando);
//Busca binaria
int bbin(nodo_t *nodoAtual, Hash numero);

//Com o usuário
void imprimeMenu();

void setAtributos(int nChar, int atributo, int ordem);

int addArquivo(FILE *arquivo);

void insereArquivo(nodo_t* &arvore, char *nomeDoArquivo);
/*
    Se a árvore esta vazia vai chamar a função leituraArquivo();
    Se Não vai carregar o arquivo um memória e chamar a função CarregaIndices().
    Após carregar os nodos para cada item do index vai ser chamada a função insere()
    passando um nodo e inserindo na árvore;
*/

void insereLinha(nodo_t* &arvore, char *linha);
/*
    Recebe a árvore e uma linha a ser inserida, como não tem o offset para referenciar no nodo,
    vai salvar em arquivo esta string e em seguida chamar  a função insereArquivo()
    passando o nome do arquivo para ser carregado e inserido.
*/

void carregaIndices(vind &indices, FILE *arquivo);
/*
    Vai receber o arquivo que já está em memória e carregar os nodos para a estrutura do tipo vind.
*/

void insere(nodo_t* &arvore, nodo_t *noAtual);
/*
    Recebe a árvore e o nodo a ser inserido, dentro vai inserir na árvore e tratar todas as exceções.
*/

