#include<stdio.h>
#include<string.h>
//#include <stdbool.h>

#define LISTA_BLOCK_FREE "LB\0"

// ============================== ESTRUTURAS ==============================
struct inode_extend
{
	int b_diretos[5];
}; 
typedef struct inode_extend InodeExtend;

struct inode_principal
{
	char permissoes[10];
	char data[12];
	char hora[9];
	int tamanho;
	char nome_usuario[100];
	char nome_grupo[100];
	int contador_link_hard;
	
	int b_diretos[5];
	int b_indi_simples;
	int b_indi_duplo;
	int b_indi_triplo;
};
typedef struct inode_principal InodePrincipal;

struct diretorio
{
	int tl;
	
	/* 
		10 ENTRADAS PARA ARQUIVO/DIRETORIO
		2 ENTRADAS PARA DIRETORIO ATUAL E ANTERIOR
		1 ENTRADA PARA EXTENSAO DE DIRETORIO
	*/
	char  nome_arq[13][255];
	int i_numero[13];
	
	struct inode_principal dir_extend;
};
typedef struct diretorio Diretorio;


struct link_simbolico
{
	
};
typedef struct link_simbolico LinkSimb;


struct no_lista_block
{
	int tl;
	int pilha[10];
	int prox;
};


struct block
{
	int endereco_disco;
	
	char tipo[3];
	
	//ESTRUTURA DE INODE, PODE SER DIRETORIO, PODE SER ARQUIVO OU PODE SER LINK HARD
	struct inode_principal inode;
	
	//ESTRUTURA DE INODE EXTENDIDO
	struct inode_extend inode_extend;
	
	//ESTRUTURA DE DIRETORIO
	struct diretorio diretorio;
	
	//ESTRTURA PARA PILHA DE BLOCOS LIVRES
	struct  no_lista_block no_lista_block;
	
	//ESTRUTURA DE LINK SIMBOLICO
	struct link_simbolico link_simb;
};
typedef struct block Block;

void init_blocos_livres(Block disco[], int total_blocos)
{
	int qnt_blocos, 
		inicio_endereco_block_livres, 
		fim_blocos,
		i,
		j;
	
	inicio_endereco_block_livres = 0;
	qnt_blocos = total_blocos;
	for(i=0 ; i < total_blocos-1 ; i++)
		for(j=0 ; j < 10 && qnt_blocos > 0 ; j++)
		{
			inicio_endereco_block_livres++;
			qnt_blocos--;		
		}
	
	
	fim_blocos = inicio_endereco_block_livres;
	for(i=0 ; i < fim_blocos; i++)
		for(j=0 ; j < 10 && inicio_endereco_block_livres < total_blocos ; j++)
		{
			strcpy(disco[i].tipo, LISTA_BLOCK_FREE);
			disco[i].no_lista_block.pilha[j] = inicio_endereco_block_livres;
			disco[i].no_lista_block.tl++;
			inicio_endereco_block_livres++;
		}
}

void exibir_blocos_livres(Block disco[], int topo_blocks_free)
{
	int i,
		j;
		
	for(i = topo_blocks_free ; strcmp(disco[i].tipo, LISTA_BLOCK_FREE) == 0 ; i++)
	{
		for(j = disco[i].no_lista_block.tl ; j > -1  ; j--)
			printf("ENDERECO => %d\n", disco[i].no_lista_block.pilha[j]);
	}
		
}


int main()
{	
	//TAMANHO FISICO DISCO
	int tf_disco = 1000;
	
	//TOPO DA LISTA DE BLOCKS
	int topo_blocks_free=0;
	
	//DISCO INTEIRO
	Block disco[1000];	
	
	//INICIAR LISTA DE PILHA DE BLOCOS COM ENDERECOS
//	init_blocos_livres(disco, tf_disco);
	exibir_blocos_livres(disco, topo_blocks_free);
	
//	ler_comando();
}

////
////void exibir_lista_pilha(ListaPilha lp[], int tl_lista, int tl_disco)
////{
////	
////	int i, j;
////	for(i=0 ; i < tl_lista ; i++)
////	{
////		printf("lista n: %d => ", i);
////		for(j=0 ; j < tl_disco/100 ; j++)
////			printf("[%d]-", lp[i].pilha[j].endereco_disco);
////			
////		printf("\n");	
////	}
////}

////char pop_lista_pilha(ListaPilha lp[], int * tl_lista, Block * block)
////{
////	int tl_block_top;
////	
////	if(*tl_lista == 0)
////		return 0;
////	
////	//PEGA TL DA PILHA DE BLOCO DISPONIVEL	
////	tl_block_top = lp[*tl_lista-1].tl;
////	
////	//PEGA BLOCO DO TOP DA PILHA DA LISTA
////	*block = lp[*tl_lista-1].pilha[tl_block_top-1];
////	
////	//DECREMENTA TL DA PILHA 
////	lp[*tl_lista-1].tl--;
////	
////	//DIMINUI O TL, POIS ACABOU UMA PILHA DE BLOCOS NA LISTA
////	if(lp[*tl_lista-1].tl == 0)
////		(*tl_lista)--;
////		
////	return 1;	
////}

////void push_lista_pilha(ListaPilha lp[], int * tl_lista, Block block)
////{
////	int tl_block_top;
////	
////	/*
////		TEM ESPACO NA PILHA DE LISTA
////		DEVOLVE O BLOCO PARA PILHA
////		INCREMENTA A PILHA
////	*/
////	if(*tl_lista < 10)
////	{
////		tl_block_top = lp[*tl_lista-1].tl;
////		lp[*tl_lista-1].pilha[tl_block_top-1] = block;
////		(*tl_lista)++;
////	}
////	
////	/*
////		NAO TEM ESPACO NA LISTA DE PILHA
////		INCREMENTA O TL DA LISTA
////		REINICIA O TL DESSA NOVA PILHA
////		DEVOLVE O BLOCK A PILHA
////	*/
////	else
////	{
////		(*tl_lista)++;
////		lp[*tl_lista-1].tl=1;
////		tl_block_top = lp[*tl_lista-1].tl;
////		lp[*tl_lista-1].pilha[tl_block_top-1] = block;
////		
////		
////	}
////}

////char reconhecer_comandos(char comando[])
////{
////		
////}

////int espaco_livre_disco(ListaPilha lp[], int tl_lista, int tf_disco)
////{
////	int quantidade;
////	int tf_pilha;
////	
////	//SE TIVEr ALGUM ESPACO
////	if(tl_lista > 0)
////	{
////		//TAMANHO FISICO DE TODAS PILHAS
////		tf_pilha = tf_disco/100;
////		
////		/*
////			SE TIVER MAIS DE UMA LISTA
////			QUANTIDADE DE LISTA * QUANTIDADE DE BLOCO EM CADA PILHA
////		*/
////		if(tl_lista > 1)
////			quantidade = (tl_lista-1) * tf_pilha;
////			
////		//QUANTIDADE DE BLOCOS DA PILHA DO TOPO DA LISTA	
////		quantidade = quantidade + lp[tl_lista-1].tl;
////		
////		return quantidade;
////	}
////	
////	return 0;
////}

////char espaco_suficiente(ListaPilha lp[], int tl_lista, int tf_disco, int quantidade_blocos)
////{
////	//VERIFICA ESPACO NECESSARIO PARA UMA QUANTIDADE DE BLOCOS
////	return espaco_livre_disco(lp, tl_lista, tf_disco) >= quantidade_blocos;
////}

////void inserir_block_disco(Block disco[1000], Block block)
////{
////	disco[block.endereco_disco] = block;
////}

////void inserir_inode_extend_simples(ListaPilha lp[], int * tl_lista, Block disco[1000], 
////							Block block_indireto_simples, int *quantidade_block_indireto)
////{
////	int i;
////	Block block_livre;
////	
////	for(i=0 ; i < 5 && *quantidade_block_indireto > 0; i++, (*quantidade_block_indireto)--)
////	{
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		block_indireto_simples.inode_extend.b_diretos[i] = block_livre.endereco_disco;
////		inserir_block_disco(disco, block_livre);
////	}
////}

////void inserir_inode_extend_duplo(ListaPilha lp[], int * tl_lista, Block disco[1000], 
////							Block block_indireto_duplo, int *quantidade_block)
////{
////	int i, blocos_restantes;
////	Block block_livre;
////	
////	for(i=0 ; i < 5 && *quantidade_block > 0 ; i++)
////	{
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		block_indireto_duplo.inode_extend.b_diretos[i].endereco_disco = block_livre.endereco_disco;
////		inserir_block_disco(disco, block_livre);
////		
////		inserir_inode_simples(lp, &*tl_lista, disco, block_livre, &*quantidade_block);
////	}
////}*/

////void inserir_inode_extend_triplo(ListaPilha lp[], int * tl_lista, Block disco[1000], 
////							Block block_indireto_triplo, int *quantidade_block)
////{
////	int i, j, blocos_restantes;
////	Block block_livre;
////	
////	for(i=0 ; i < 5 && *quantidade_block ; i++)
////	{
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		block_indireto_triplo.inode_extend.b_diretos[i] = block_livre.endereco_disco;
////		inserir_block_disco(disco, block_livre);
////		
////		inserir_inode_duplo(lp, &*tl_lista, disco, block_livre, &*quantidade_block);
////	}
////}*/

////void inserir_inode_arquivo( ListaPilha lp[], int * tl_lista, Block disco[1000], 
////							int endereco_block, int quantidade_blocos)
////{
////	/* 
////		PARA USAR ESSA FUNCAO, EH NECESSARIO VERIFICAR ESPACO EM DISCO PARA 
////		A QUANTIDADE DE BLOCOS QUE VAI NO ARQUIVO.
////	
////		lp[] => LISTA DE PILHA DA BLOCOS LIVRES 
////		tl_lista => TAMANHO LOGICO DE lp[]
////		disco[1000] => DISCO INTEIRO
////		endereco_block => POSICAO QUE VAI O ARQUIVO NO DISCO
////		quantidade_blocos => QUANTIDADE DE BLOCOS PARA O ARQUIVO, FORA O PROPRIO ARQUIVO
////	*/
////	
////	Block block, block_livre, block_aux;
////	int i, blocos_alocados;
////	
////	//ARQUIVO REGULAR
//////	strcpy(block.tipo, "IP");
////	block.endereco_disco = endereco_block;
////	block.inode.tamanho = quantidade_blocos * 10;
////	strcpy(block.inode.permissoes, "-RWXRWXRWX");
////	strcpy(block.inode.data, __DATE__);
////	strcpy(block.inode.hora, __TIME__);
////	strcpy(block.inode.nome_usuario, NOME_USUARIO_PADRAO);
////	strcpy(block.inode.nome_grupo, NOME_GRUPO_PADRAO);
////	block.inode.contador_link_hard = 0;
////	
////	inserir_block_disco(disco, block);
////	
////	//INSERIR OS BLOCOS DIRETOS QNTD = 5
////	for(i=0; i < quantidade_blocos && i < 5 ; i++, quantidade_block--)
////	{
////		//PEGA BLOCK DA LISTA DE BLOCOS
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		
////		//ATRIBUI ENDERECO DO BLOCO NA VARIAVEL QUE APONTA
////		block.inode.b_diretos[i] = block_livre.endereco_disco;
////		
////		//ALOCA O BLOCO REGULAR NO DISCO
////		inserir_block_disco(disco, block_aux);
////	}
////	
////	//BLOCOS INDIRETOS SIMPLES QNTD = 10
////	if(quantidade_blocos)
////	{
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		block.inode.b_indi_simples = block_livre.endereco_disco;
////		inserir_block_disco(disco, block_livre);
////	
////		inserir_inode_extend_simples(disco, block_livre, quantidade_blocos);	
////	}
////	

////	//BLOCOS INDIRETOS DUPLO QNTD = 125
////	if(quantidade_blocos)
////	{
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		block.inode.b_indi_duplo = block_livre.endereco_disco;
////		inserir_block_disco(disco, block_livre);
////		
////		inserir_inode_extend_duplo(disco, block_livre, quantidade_blocos);
////	}
////	//BLOCO INDIRETO TRIPLO QNTD = 625
////	if(quantidade_blocos)
////	{
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		block.inode.b_indi_triplo = block_livre.endereco_disco;
////		inserir_block_disco(disco, block_livre);
////		
////		inserir_inode_extend_triplo(disco, block_livre, quantidade_blocos);
////	}
////	
////	//EXTENSAO, COMO FAZER?
////	if(quantidade_blocos)
////	{
////		//O QUE FAZER?
////	}
////}

////void remover_inode_arquivo( ListaPilha lp[], int * tl_lista, Block disco[1000], int endereco_block )
////{
////	Block block_remove, block_direto;
////	int i, quantidade_blocos;
////	
////	block_remove = lp[*endereco_block];
////	quantidade_blocos = (block_remove.tamanho/10)
////	
////	for(i=0 ; i < 5 && quantidade_blocos ; i++, quantidade_blocos--)
////	{
////		disco[block_remove.b_diretos[i]].endereco_disco = 'F';
////		block_remove = disco[block_remove.b_diretos[i]];
////		push_lista_pilha(lp, tl_lista, block_remove);
////	}
////	
////	//BLOCOS INDIRETOS SIMPLES QNTD = 10
////	if(quantidade_blocos)
////	{
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		block.inode.b_indi_simples = block_livre.endereco_disco;
////		inserir_block_disco(disco, block_livre);
////	
////		inserir_inode_extend_simples(disco, block_livre, quantidade_blocos);	
////		//TEM QUE MOREVER A PROPRIA ESTRTUTURA 
////	}
////	

////	//BLOCOS INDIRETOS DUPLO QNTD = 125
////	if(quantidade_blocos)
////	{
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		block.inode.b_indi_duplo = block_livre.endereco_disco;
////		inserir_block_disco(disco, block_livre);
////		
////		inserir_inode_duplo(disco, block_livre, quantidade_blocos);
////		//TEM QUE MOREVER A PROPRIA ESTRTUTURA 
////	}
////	//BLOCO INDIRETO TRIPLO QNTD = 625
////	if(quantidade_blocos)
////	{
////		pop_lista_pilha(lp, tl_lista, &block_livre);
////		block.inode.b_indi_triplo = block_livre.endereco_disco;
////		inserir_block_disco(disco, block_livre);
////		
////		inserir_inode_triplo(disco, block_livre, quantidade_blocos);
////		//TEM QUE MOREVER A PROPRIA ESTRTUTURA 
////	}
////	
////	//EXTENSAO, COMO FAZER?
////	if(quantidade_blocos)
////	{
////		//O QUE FAZER?
////	}
////}

////void inserir_arquivo_diretorio(Block block_arquivo, int inode_numero, char nome_arq[255])
////{
////	/*
////		ESTA FUNCAO INSERE UM ARQUIVO NUMA ESTRUTURA DE DIRETORIO
////		NA POSICAO TL É INSERIDO O NUMERO DO INODE E O NOME DO ARQUIVO
////	*/
////	int tl_estrutura_dir;
////	
////	tl_estrutura_dir = block_arquivo.diretorio.tl;
////	
////	if(tl_estrutura_dir < 10)
////	{
////		
////		block_arquivo.diretorio.i_numero[tl_estrutura_dir] = inode_numero; 
////		strcpy(block_arquivo.diretorio.nome_arq[tl_estrutura_dir], nome_arq); 
////		
////		block_arquivo.diretorio.tl++;
////		
////	}
////	int tl_block_top;
////	
////	//push_lista_pilha(lp, tl_lista, block);
////	
////	if(*tl_dir < 13)
////	{
////		tl_block_top = dir[*tl_dir-1].tl;
////		dir[*tl_dir-1].nome_arq = block.diretorio.nome_arq;
////		dir[*tl_dir-1].i_numero = block.diretorio.i_numero;
////		(*tl_dir)++;
////	}
////	else
////	{
////		printf("\n- Diretorio cheio. Criando novo diretorio.");
////		Block b = criar_diretorio("Novo diretorio");
////		b.diretorio.tl = 1;
////		dir[0].nome_arq = block.diretorio.nome_arq;
////		dir[0].i_numero = block.diretorio.i_numero;
////	}
////}

////função - chmod 
//void alterar_permissao (char nome[15])
//{
//	//nome começa da posição 1, pois o 0 é o sinal
//}

////função - Is
//void listar_nomes_arq_dir ()
//{
//	
//}

////função - Is -I
//void listar_nomes_arq_atributos()
//{
//	
//}

////função - df
//void espaco_livre_ocupado()
//{
//	
//}

////função - vi nomeArquivo
//void visualizar_arquivo_regular(char arq[15])
//{
//	
//}

////função - mkdir
//void criar_diretorio(char nome[15])
//{
//	
//}

////função rmdir
//void deletar_diretorio_vazio(char nome[15])
//{
//	
//}

////função - rm
//void deletar_arquivo(char nome[15])
//{
//	
//}

////fução cd
//void cd(char nome[15], char n[3])
//{
//	
//}

////função -  link –h  
//void link_fisico(char origem[15], char destino[15])
//{
//	
//}

////função -  link –s
//void link_simbolico(char origem[15], char destino[15])
//{
//	
//}

////função unlink - h
//void unlink_fisico()
//{
//	
//}

////função unlink - s
//void unlink_simbolico()
//{
//	
//}

////função bad
//void bad(int num)
//{
//	
//}

////função touch
//void touch(char nome[15], int num)
//{
//	
//}


//void ler_comando()
//{
//	char linha[30], comando[15], nome[15], n1[15], n2[15];
//	int i, j, num;
//	bool ok;
//	
//	while(strcmp(comando,"exit") != 0)
//	{

//		printf("- ");
//		scanf("%[^\n]s",&linha);
//		setbuf(stdin, NULL);
//		
//		for(i = 0; linha[i] != '\0'; i++)
//			linha[i] = tolower(linha[i]);
//		
//		for(i = 0; linha[i] != '\0'; )
//		{
//			for(j = 0; linha[i] !=' ' && linha[i] !='\0'; j++)
//				comando[j] = linha[i++];
//			comando[j] = '\0';
//				
//			if(linha[i] !='\0')
//				i++;
//			
//			for(j = 0; linha[i] !=' ' && linha[i] !='\0'; j++)
//				nome[j] = linha[i++];
//			nome[j] = '\0';
//				
//			if(linha[i] !='\0')
//				i++;
//				
//			for(j = 0; linha[i] !=' ' && linha[i] !='\0'; j++)
//				n1[j] = linha[i++];
//			n1[j] = '\0';
//			
//			if(linha[i] !='\0')
//				i++;
//				
//			for(j = 0; linha[i] !=' ' && linha[i] !='\0'; j++)
//				n2[j] = linha[i++];
//			n2[j] = '\0';
//		}
//	
//	
//		if(strcmp(comando,"is") != 0 && strcmp(comando,"df") != 0)
//		{
//			if(strcmp(comando,"chmod") == 0)
//				alterar_permissao(nome);
//			else
//				if(strcmp(comando,"vi") == 0)
//					visualizar_arquivo_regular(nome);
//				else
//					if(strcmp(comando,"mkdir") == 0)	
//						criar_diretorio(nome);
//					else
//						if(strcmp(comando,"rmdir") == 0)	
//							deletar_diretorio_vazio(nome);	
//						else
//							if(strcmp(comando,"rm") == 0)	
//								deletar_arquivo(nome);
//							else
//								if(strcmp(comando,"cd") == 0)
//								{
//									if(strcmp(n1, ".") == 0 || strcmp(n1, "..") == 0)
//										cd(nome, n1);
//									else
//										printf("- Forma de navegacao invalida.");
//								}
//								else
//									if(strcmp(comando,"link") == 0)
//									{
//										if(strcmp(nome, "-h") == 0 && strcmp(n1, "") != 0 && strcmp(n2, "") != 0)
//											link_fisico(n1,n2);
//										else
//											if(strcmp(nome, "-s") == 0 && strcmp(n1, "") != 0 && strcmp(n2, "") != 0)
//												link_simbolico(n1,n2);
//											else
//												printf("- Opcao de link  ou nomes de arquivos invalidos.");
//									}
//									else
//										if(strcmp(comando,"unlink") == 0)
//										{
//											if(strcmp(nome, "-h") == 0)
//												unlink_fisico();
//											else
//												if(strcmp(nome, "-s") == 0)
//													unlink_simbolico();
//												else
//													printf("- Opcao de link invalida.");
//										}
//										else
//											if(strcmp(comando,"bad") == 0)
//											{
//												ok = true;
//												for(j = 0; nome[j] !='\0'; j++)
//													if(nome[j] != '0' && nome[j] != '1' && nome[j] != '2' && nome[j] != '3' && nome[j] != '4' && nome[j] != '5' && nome[j] != '6' && nome[j] != '7' && nome[j] != '8' && nome[j] != '9')
//														ok = false;
//												if(ok)
//												{
//													num = atoi(nome);
//													bad(num);
//												}
//												else
//													printf("- Numero invalido.");
//											}
//											else
//												if(strcmp(comando,"touch") == 0)	
//												{
//													ok = true;
//													for(j = 0; nome[j] !='\0'; j++)
//														if(nome[j] != '0' && nome[j] != '1' && nome[j] != '2' && nome[j] != '3' && nome[j] != '4' && nome[j] != '5' && nome[j] != '6' && nome[j] != '7' && nome[j] != '8' && nome[j] != '9')
//															ok = false;
//													
//													if(ok)
//													{
//														num = atoi(nome);
//														touch(nome, num);
//													}
//													else
//														printf("- Numero invalido.");
//												}
//												else
//													printf("- Opcao invalida");	
//		}
//		else
//		{
//			if(strcmp(comando,"ls") == 0)
//			{
//				if(strcmp(nome,"")==0)
//					listar_nomes_arq_dir();
//				else
//					if(strcmp(nome,"-l")==0)
//						listar_nomes_arq_atributos();
//					else
//						printf("- Opcao invalida.");
//			}	
//			else
//				espaco_livre_ocupado();
//		}
//		printf("\n");

//	}
//}


