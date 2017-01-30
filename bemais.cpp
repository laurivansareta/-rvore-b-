#ifndef _BEMAIS_
#define _BEMAIS_
#include "bemais.h"
#include<time.h>
using namespace std;


/*
keys minimas = (ordem-1)/2
filhos minimos = (ordem+1)/2
 */

int _nChar = 0;
int _atributo = 0;
int _ordem = 0;
FILE **_arquivos;


Hash hashFunction(char *str) { //funcao hash djb2
  Hash hash = 5381;
  int c;
  while ((c = *(str++))) { hash = ((hash << 5) + hash) + c; }
  return hash;
}

void leituraArquivo(vind &indices, FILE *entrada){
  int tamanho, i, j, virgula;
  char linha[MAXLINHA], aux[_nChar+1];
  Hash hash = 0;
  Offset offsetAux;

  while (offsetAux = ftell(entrada), fgets(linha, MAXLINHA, entrada)) {
    virgula = _atributo - 1;
    linha[strlen(linha)-1] = '\0';
    //passa o atributo pro aux
    for (i = 0, tamanho = strlen(linha); i < tamanho && virgula; i++)
      if (linha[i] == ',') virgula--;
    for (j = 0; j < _nChar && linha[i] != '\0' && linha[i] != '\n' && linha[i] != ','; i++)
      if (linha[i] != '"') aux[j++] = linha[i];
    aux[j] = '\0';

    //faz hash
    hash = hashFunction(aux);

    //empurra no vetor
    indices.push_back(index_t(hash, offsetAux));
  }

  //ordena de acordo com as hashs
  sort(indices.begin(), indices.end(), compareIndex);
}

FILE* abrirArquivo(char arquivoEntrada[]){
  FILE *entrada;
  entrada = fopen(arquivoEntrada, "a+"); //abre o arquivo de entrada que vai ser passado de parametro quando executar o programa
  if (!entrada) { printf("Não abriu entrada\n"); return NULL; }
  fseek(entrada, 0, SEEK_SET); //retorna o buffer pro começo (so pra garantir)
  return entrada;
}

bool compareIndex(const index_t &_a, const index_t &_b) {
  return _a.hash < _b.hash;
}

nodo_t* trataExcecoes(nodo_t* paiAtual, nodo_t *filhoAtual, int ordem) {
  //excecão: único filho tem menos entrada que o mínimo da ordem
  if (!paiAtual->pai && paiAtual->quantidadeFilhos == 1 && filhoAtual) {
    filhoAtual->pai = NULL;
    paiAtual->quantidadeFilhos = 0;
    mataArvore(paiAtual);
    return filhoAtual;
  }
  //excecão: último filho tem menos que a ordem ↓
  if (filhoAtual && filhoAtual->quantidadeKeys < (ordem-1)/2) {
    nodo_t *familiarBeneficiario = NULL;
    int i, j;

    //acha o parente que vai receber as keys e os offsets
    if (paiAtual->quantidadeFilhos > 1) familiarBeneficiario = paiAtual->filhos[paiAtual->quantidadeFilhos-2];
    else { //else: pega do primo
      nodo_t *voAtual = paiAtual->pai;
      nodo_t *tioAtual = voAtual->filhos[voAtual->quantidadeFilhos-2];
      familiarBeneficiario = tioAtual->filhos[tioAtual->quantidadeFilhos-1];
    }
    for (i = familiarBeneficiario->quantidadeKeys, j = 0; j < filhoAtual->quantidadeKeys; i++, j++) {
      familiarBeneficiario->keys[i] = filhoAtual->keys[j];
      familiarBeneficiario->offsets[i] = filhoAtual->offsets[j];
      familiarBeneficiario->quantidadeKeys++;
    }
    filhoAtual->quantidadeKeys = 0; //coloca 0 se nao o mataArvore vai apagar todos os offsets que agora estao no familiarBeneficiario
    //paiAtual->quantidadeFilhos--;
    //paiAtual->quantidadeKeys--;
    mataArvore(filhoAtual);
    filhoAtual = familiarBeneficiario;
    removeUltimo(paiAtual, ordem); //remove um de todos os ancestrais do antigo filhoAtual
  }
  return filhoAtual;
}

int bulk_loading(nodo_t* &arvore, vind &indices, int ordem, int indexArquivo){
  nodo_t *filhoAtual = NULL, *paiAtual = NULL;
  offsets_t *novo;
  int iteradorIndices = 0, first = 1, condicaoParaFor = (ordem-1)/2;

  //cria o primeiro pai
  paiAtual = criaNodo(ordem, false);
  if (!paiAtual) { printf("Erro criando o primeiro pai\n"); return 1; }

  while (iteradorIndices < (int)indices.size()) {
    filhoAtual = criaNodo(ordem, true);

    //preenche o nodo filhoAtual, que é o filho do paiAtual
    for(int j = 0; j <= condicaoParaFor && iteradorIndices < (int)indices.size(); iteradorIndices++, j++){
      //se o hash a ser inserido for o mesmo que o anterior, ele coloca o offset no mesmo
      if (j && filhoAtual->keys[j-1] == indices[iteradorIndices].hash) {
        j--;
      } else if (j == condicaoParaFor && indices[iteradorIndices].hash != filhoAtual->keys[j-1]) {
        break;
      } else {
        filhoAtual->keys[j] = indices[iteradorIndices].hash;
        filhoAtual->quantidadeKeys++;
      }

      //cria novo offset, passando como parametro o offset da hash atual e ligando o novo offset no começo da lista
      novo = NULL;
      novo = criaOffset(indices[iteradorIndices].offset, filhoAtual->offsets[j], indexArquivo);
      if (!novo) { printf("Erro ao criar offset %lld", indices[iteradorIndices].offset); return 1; }
      filhoAtual->offsets[j] = novo;
    }

    if (first) { //primeiro caso/folha
      first = 0;
      paiAtual->filhos[0] = filhoAtual;
      filhoAtual->pai = paiAtual;
      paiAtual->quantidadeFilhos = 1;
    } else if (checaPai(filhoAtual, &paiAtual, filhoAtual->keys[0], ordem)) {
      return 1;
    }
  }

  filhoAtual = trataExcecoes(paiAtual, filhoAtual, ordem);

  //atualiza a raiz
  while (filhoAtual->pai != NULL) filhoAtual = filhoAtual->pai;
  arvore = filhoAtual;
  return 0;
}

int checaPai(nodo_t *filhoAtual, nodo_t** pAtual, Hash hashQueVem, int ordem) { //checa se precisa trocar o pai atual
  nodo_t *tioAtual = NULL, *voAtual = (*pAtual)->pai, *paiAtual = *pAtual;
  Hash hashQueSobe;
  int i, j;

  if (paiAtual->quantidadeFilhos >= ordem) {
    tioAtual = criaNodo(ordem, false);
    if (!tioAtual) return 1; //retorna erro se nao criou

    //pega hash que sobe
    hashQueSobe = paiAtual->keys[(ordem-1)/2];

    //divide paiAtual com tioAtual
    for (i = (ordem + 1) / 2, j = 0; i < ordem - 1; i++, j++) {
      tioAtual->keys[j] = paiAtual->keys[i];
      tioAtual->filhos[j] = paiAtual->filhos[i];
      tioAtual->filhos[j]->pai = tioAtual;
    }
    tioAtual->filhos[j] = paiAtual->filhos[i];
    tioAtual->filhos[j]->pai = tioAtual;
    //coloca novos numeros
    tioAtual->quantidadeKeys = (ordem/2) - 1;
    tioAtual->quantidadeFilhos = tioAtual->quantidadeKeys + 1;
    paiAtual->quantidadeKeys = (ordem-1)/2;
    paiAtual->quantidadeFilhos = (ordem+1)/2;

    if (!paiAtual->pai) {
      voAtual = criaNodo(ordem, false);
      paiAtual->pai = voAtual;
      voAtual->filhos[0] = paiAtual;
      voAtual->quantidadeFilhos = 1;
    }
    checaPai(tioAtual, &voAtual, hashQueSobe, ordem);
    paiAtual = tioAtual;
  }

  //coloca hashQueVem no final
  paiAtual->keys[paiAtual->quantidadeKeys++] = hashQueVem;
  paiAtual->filhos[paiAtual->quantidadeFilhos++] = filhoAtual;
  filhoAtual->pai = paiAtual;
  *pAtual = paiAtual;
  return 0;
}

nodo_t* criaNodo(int ordem, bool folha){
  nodo_t *nodo = NULL;

  nodo = (nodo_t*)malloc(sizeof(nodo_t));
  if (!nodo) { printf("Erro na criaNodo\n"); return NULL; }

  if(!folha) {
    nodo->filhos = NULL;
    nodo->filhos = (nodo_t**)malloc( sizeof(nodo_t*) * ordem );
    if (!nodo->filhos) { printf("Erro inicializando vetor dos filhos\n"); return NULL; }
    nodo->offsets = NULL;
  } else {
    nodo->offsets = NULL;
    nodo->offsets = (offsets_t**)malloc(sizeof(offsets_t*)*(ordem-1));
    if (!nodo->offsets) { printf("Erro inicializando vetor de offsets\n"); return NULL; }
    nodo->filhos = NULL;
    memset(nodo->offsets, 0, sizeof(offsets_t*)*(ordem-1));
  }
  nodo->keys = NULL;
  nodo->keys = (Hash*)malloc(sizeof(Hash)*(ordem-1));
  if (!nodo->keys) { printf("Erro inicializando vetor das chaves\n"); return NULL; }
  nodo->quantidadeKeys = nodo->quantidadeFilhos = 0;
  nodo->prox = nodo->pai = NULL;
  nodo->folha = folha;
  return nodo;

}

offsets_t* criaOffset(Offset o, offsets_t *p, int indexArquivo) {
  offsets_t *r = NULL;
  r = (offsets_t*)malloc(sizeof(offsets_t));
  r->offset = o;
  r->indexArquivo = indexArquivo;
  r->prox = p;
  return r;
}

void mataArvore(nodo_t *n) {
  if (!n) return;
  for (int i = 0; n->folha && i < n->quantidadeKeys; i++) {
    if (n->offsets[i]) mataOffsets(n->offsets[i]->prox);
  }

  for (int i = 0; !n->folha && i < n->quantidadeFilhos; i++) {
    mataArvore(n->filhos[i]);
  }

  if (n->filhos) free(n->filhos);
  if (n->offsets) free(n->offsets);
  free(n->keys);
}

void mataOffsets(offsets_t *o) {
  if (!o) return;
  mataOffsets(o->prox);
  free(o);
}

int imprimeArvore(nodo_t *arvore) {
  FILE *dotFile = NULL;
  char nomeArquivo[] = "saida.dot", comando[400];
  int numeroNodo = 0;

  //cria o comando
  sprintf(comando, "dot %s -Tpng -o saida.png && kde-open saida.png\n", nomeArquivo);

  //abre dotFile
  dotFile = fopen(nomeArquivo, "w");
  if (!dotFile) { printf("Erro abrindo %s\n", nomeArquivo); return 1; }
  fprintf(dotFile, "graph {\n");
  imprimeNodos(dotFile, arvore, &numeroNodo, 0);
  fprintf(dotFile, "}\n");
  fclose(dotFile);
  system(comando);
  return 0;
}

void imprimeNodos(FILE *dotFile, nodo_t *n, int *numeroNodo, int liga) {
  int esseNumero = *numeroNodo;
  if (!n) return;
  fprintf(dotFile, "%d [label=%d];\n", (*numeroNodo)++, liga);
  for (int i = 0; i < n->quantidadeFilhos; i++) {
    fprintf(dotFile, "%d -- ", esseNumero);
    (*numeroNodo)++;
    imprimeNodos(dotFile, n->filhos[i], numeroNodo, i);
  }

  //imprime esse nodo
  fprintf(dotFile, "%d [label=\"", esseNumero);
  for (int i = 0; i < n->quantidadeKeys; i++)
    fprintf(dotFile, "%s %lld", i? " »":"", n->keys[i]);
  fprintf(dotFile, "\"];\n");
  fprintf(dotFile, "%d [shape=box]\n", esseNumero);
}

void removeUltimo(nodo_t *paiAtual, int ordem) {
  if (paiAtual->quantidadeKeys > (ordem-1)/2 || paiAtual->pai == NULL) { //se ele tem suficiente ou é raiz
    paiAtual->quantidadeKeys--;
    paiAtual->quantidadeFilhos--;
    if (paiAtual->quantidadeKeys <= 0) {
      paiAtual->filhos[0]->pai = NULL;
      mataArvore(paiAtual);
    }
    return;
  }
  nodo_t *tioAtual, *voAtual;
  int i, j;
  voAtual = paiAtual->pai;
  tioAtual = voAtual->filhos[--voAtual->quantidadeFilhos - 1];

  //abaixa uma hash pro tio
  tioAtual->keys[tioAtual->quantidadeKeys++] = voAtual->keys[--voAtual->quantidadeKeys];
  printf("%d]", tioAtual->quantidadeKeys);

  for (i = tioAtual->quantidadeKeys, j = 0; j < paiAtual->quantidadeKeys; j++, i++) {
    paiAtual->filhos[j]->pai = tioAtual;
    tioAtual->keys[i] = paiAtual->keys[j];
    tioAtual->filhos[i] = paiAtual->filhos[j];
    tioAtual->quantidadeKeys++;
    tioAtual->quantidadeFilhos++;
  }
  tioAtual->filhos[i] = paiAtual->filhos[j];
  paiAtual->filhos[j]->pai = tioAtual;
  tioAtual->quantidadeFilhos++;

  paiAtual->quantidadeFilhos = paiAtual->quantidadeKeys = 0;
  mataArvore(paiAtual);
  removeUltimo(voAtual, ordem);

}

int bbin(nodo_t *nodoAtual, Hash numero) {
  int li = 0, ls = nodoAtual->quantidadeKeys;
  while (li < ls) {
    int meio = (li + ls) / 2;
    if (nodoAtual->keys[meio] < numero) li = meio + 1;
    else ls = meio;
  }
  return ls + (!nodoAtual->folha && nodoAtual->keys[ls] == numero);
}

nodo_t *achaElemento(nodo_t* noAtual, int &indice, Hash procurando) {
  int ind = bbin(noAtual, procurando);
  if (noAtual->keys[ind] == procurando && noAtual->folha) {
    indice = ind;
    return noAtual;
  }
  if (!noAtual->folha) return achaElemento(noAtual->filhos[ind], indice, procurando);
  return NULL;
}

void imprimeTupla(nodo_t *nodoAtual, int indiceElemento) {
  offsets_t *o;
  char tupla[MAXLINHA];
  FILE *arquivo = _arquivos[nodoAtual->offsets[indiceElemento]->indexArquivo];
  if (!nodoAtual || !arquivo || indiceElemento < 0) return;
  for (o = nodoAtual->offsets[indiceElemento]; o; o = o->prox) {
    fseek(arquivo, o->offset, SEEK_SET);
    fgets(tupla, MAXLINHA, arquivo);
    if (tupla[strlen(tupla)-1] == '\n') tupla[strlen(tupla)-1] = '\0';
    printf("%s\n", tupla);
  }
}

int addArquivo(FILE *arquivo){
  int i;

  if (_arquivos == NULL){
    _arquivos = (FILE **)malloc(1000*sizeof(FILE));
  }
  for(i=0; i<MAXARQUIVOS; i++){
    if (_arquivos[i] == NULL) break;
  }
  if (i==MAXARQUIVOS) {
    printf("Erro ao salvar arquivo em memória, tem mais arquivos do que a estrutura suporta\n");
    return -1;
  }
  _arquivos[i] = arquivo;
  return i;
}

char *criaArquivo(char *texto){
  char *nomeArquivo;
  FILE *arq;
  time_t t;
  time(&t);

  nomeArquivo = ctime(&t);

  //abrindo o arquivo
  arq = fopen(nomeArquivo, "a");
  fprintf(arq, texto);
  fputc('\n', arq);
  fclose(arq);

  printf("\nO arquivo \%s foi  criado com sucesso!", nomeArquivo);
  return nomeArquivo;
}

void insereLinha(nodo_t* &arvore, char *linha){
  insereArquivo(arvore, criaArquivo(linha));
}

nodo_t *achaElementoInsercao(nodo_t* noAtual, int &indice, Hash procurando) {
  int ind = bbin(noAtual, procurando);
  //Ajustado para buscar o o índice correto para ser inserido caso não encontre um igual.
  indice = ind;
  if (noAtual->keys[ind] == procurando && noAtual->folha) {
    return noAtual;
  }
  if (!noAtual->folha) return achaElementoInsercao(noAtual->filhos[ind], indice, procurando);
  return noAtual;
}

void insere(nodo_t* &arvore, index_t info, int indexArquivo){
  nodo_t *noAtual = NULL, *noNovo = NULL, *irmaoAtual = NULL, *paiAtual = NULL;
  int indice, i, j = 0;
  offsets_t *novo, *offsetTemp, *offsetTemp2 = NULL;
  Hash hashQueSobe=0, keyTemp=0, keyTemp2=0;

  printf("\n Deu certo");
  printf("info ao criar offset %lld", info.offset);
  printf("info ao criar hash %lld \n", info.hash);
  noAtual = achaElementoInsercao(arvore, indice, info.hash);

  if (noAtual->keys[indice] != info.hash){
    for (i=indice; i<noAtual->quantidadeKeys; i++){
      if ((noAtual->keys[i] > info.hash) || (i==(noAtual->quantidadeKeys-1))){
        keyTemp = noAtual->keys[i];
        offsetTemp = noAtual->offsets[i];
        noAtual->keys[i] = info.hash;

        novo = criaOffset(info.offset, noAtual->offsets[i], indexArquivo);
        if (!novo) { printf("Erro ao criar offset %lld", info.offset); return; }
        noAtual->offsets[i] = novo;

        for (i++; i<=noAtual->quantidadeKeys;i++){
          keyTemp2 = noAtual->keys[i];
          offsetTemp2 = noAtual->offsets[i];

          noAtual->keys[i] = keyTemp;
          noAtual->offsets[i] = offsetTemp;

          keyTemp = keyTemp2;
          offsetTemp = offsetTemp2;
        }
        keyTemp = -1;
        noAtual->quantidadeKeys++;
      }
    }
    if (keyTemp == 0){
      noAtual->keys[noAtual->quantidadeKeys] = info.hash;
      novo = criaOffset(info.offset, noAtual->offsets[noAtual->quantidadeKeys], indexArquivo);
      if (!novo) { printf("Erro ao criar offset %lld", info.offset); return; }
      noAtual->offsets[noAtual->quantidadeKeys] = novo;
      noAtual->quantidadeKeys++;
    }
  }else{
    novo = criaOffset(info.offset, noAtual->offsets[noAtual->quantidadeKeys-1], indexArquivo);
    if (!novo) { printf("Erro ao criar offset %lld", info.offset); return; }
    noAtual->offsets[indice] = novo;
  }

  if (noAtual->quantidadeKeys >= _ordem) {
    irmaoAtual = criaNodo(_ordem, true);
    if (!irmaoAtual) return;

    hashQueSobe = noAtual->keys[(_ordem-1)/2];

    //divide noAtual com irmaoAtual
    for (i = (_ordem-1)/2, j = 0; i <= _ordem - 1; i++, j++) {
      irmaoAtual->keys[j] = noAtual->keys[i];
    }

    irmaoAtual->quantidadeKeys = (_ordem+1)/2;
    irmaoAtual->quantidadeFilhos = 0;

    noAtual->quantidadeKeys = (_ordem-1)/2;
    noAtual->quantidadeFilhos = 0;

    paiAtual = noAtual->pai;
    irmaoAtual->pai = paiAtual;
    paiAtual->filhos[paiAtual->quantidadeFilhos++] = irmaoAtual;
    paiAtual->keys[paiAtual->quantidadeKeys++] = hashQueSobe;
  }

  if (noAtual->pai && (noAtual->pai->quantidadeFilhos > _ordem)) {
      checaPai(irmaoAtual, &paiAtual, paiAtual->keys[(_ordem-1)/2], _ordem);
  }
}

void insereArquivo(nodo_t* &arvore, char *nomeDoArquivo){
  vind indices;
  int indiceArquivo = addArquivo(abrirArquivo(nomeDoArquivo)), i;

  if (indiceArquivo == -1) return;

  //lê as entradas o coloca os offsets das tuplas no vetor de índices
  leituraArquivo(indices, _arquivos[indiceArquivo]);

  if (arvore == NULL){
    //realiza o bulkload que retornara 0 no sucesso
    if (bulk_loading(arvore, indices, _ordem, indiceArquivo)) return;
  }else{
    //Chamar função insere para cada item;
    for (i = 0; i < indices.size(); i++){
      printf("\nIndice de insercao:%d",i);
      insere(arvore, indices[i], indiceArquivo);
    }
  }
  for (i = 0; i < (int)indices.size(); i++)
    printf("%d: %llu\n", i, indices[i].hash);
}

void setAtributos(int nChar, int atributo, int ordem){
  _nChar = nChar;
  _atributo = atributo;
  _ordem = ordem;
}

void fecharArquivos(){
  if (!_arquivos[0]) return;
  for(int i=0; i<MAXARQUIVOS; i++){
    if (_arquivos[i] == NULL) break;
    fclose(_arquivos[i]);
  }
}

void imprimeMenu() {
  printf("Digite a opção desejada\n");
  printf("0• Sair\n");
  printf("1• Imprimir a árvore\n");
  printf("2• Buscar\n");
  printf("3• Inserção por arquivo\n");
  printf("4• Inserção via terminal\n");
}

#endif
