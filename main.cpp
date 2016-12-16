#include <cstdio>
#include <cstdlib>
#include "bemais.h"

int main(int argc, char *argv[]) {
  int nChar, atributo, ordem, opMenu = 1, indiceBusca;
  char nomeArquivo[200], busca[MAXLINHA], insercao[MAXLINHA];
  nodo_t *arvore = NULL, *nodoDeBusca;

  //pega a quantidade de caracteres que farao parte do indice, a ordem da arvore, o atributo que sera levado em conta e o nome do arquivo
  atributo = atoi(argv[1]);
  nChar = atoi(argv[2]);
  ordem = atoi(argv[3]);
  strcpy(nomeArquivo,argv[4]);

  setAtributos(nChar, atributo, ordem);

  if(strlen(nomeArquivo) > 0) {
    //Carrega e insere o arquivo informado inicialmente;
    insereArquivo(arvore, nomeArquivo);
  }

  while (opMenu) {
    imprimeMenu();
    scanf("%d", &opMenu);
    getchar();
    //    system("clear\n");
    switch (opMenu) {
    case 0: break;
    case 1:
      imprimeArvore(arvore);
      break;
    case 2:
      printf("Digite o texto a ser buscado\n");
      fgets(busca, MAXLINHA, stdin);
      busca[strlen(busca)-1] = '\0';
      busca[nChar] = '\0';
      //      system("clear\n");
      printf("Você buscou: %s\n", busca);
      nodoDeBusca = achaElemento(arvore, indiceBusca, hashFunction(busca));
      if (!nodoDeBusca) printf("Linha não encontrada\n");
      else {
        printf("\nResultados encontrados:\n");
        imprimeTupla(nodoDeBusca, indiceBusca);
        putchar('\n');
      }
      break;
    case 3:
        printf("Digite nome do arquivo a ser inserido.\n");
        fgets(insercao, MAXLINHA, stdin);
        insercao[strlen(insercao)-1] = '\0';
        insercao[MAXLINHA] = '\0';
        insereArquivo(arvore, insercao);
        break;
    case 4:
        printf("Digite o texto a ser Inserido\n");
        fgets(insercao, MAXLINHA, stdin);
        insercao[strlen(insercao)-1] = '\0';
        insercao[MAXLINHA] = '\0';
        insereLinha(arvore, insercao);
        break;
    default:
      printf("Opção inválida\n");
      break;
    }
  }
  mataArvore(arvore);
  fecharArquivos();
  return 0;
}
