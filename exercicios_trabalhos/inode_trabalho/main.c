#include<stdio.h>
#include<string.h>
#include <stdbool.h>
#include "meuconio.h"

#define NOME_USUARIO_PADRAO "root\0"
#define NOME_GRUPO_PADRAO  "root\0"

// ============================== ESTRUTURAS ==============================
struct inode_extend
{
	int b_diretos[5];
};
struct inode_principal
{
	char permissoes[11];
	char data[12];
	char hora[9];
	int tamanho;
	char nome_usuario[30];
	char nome_grupo[30];
	int contador_link_hard;
	
	int b_diretos[5];
	int b_indi_simples;
	int b_indi_duplo;
	int b_indi_triplo;
};

struct diretorio
{
	int tl;
	
	/* 
		10 ENTRADAS PARA ARQUIVO/DIRETORIO
		2 ENTRADAS PARA DIRETORIO ATUAL E ANTERIOR
		1 ENTRADA PARA EXTENSAO DE DIRETORIO
	*/
	char  nome_arq[13][30];
	int i_numero[13];
	
	struct inode_principal dir_extend;
};

struct link_simbolico
{
	char caminho[255];
};

struct no_lista_block
{
	int tl;
	int pilha[10];
	int prox;
};

struct block
{	
	char tipo; 
	
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

char verificar_consistencia_arquivo(Block Disco[], int i_num);
void deletar_arquivo(Block disco[], int * topo_blocks_free, int endereco_inode_atual, int *quantidade_blocos);
char nome_igual_diretorio(Block disco[], int endereco_inode_dir_atual, char nome_arquivo[], int dir_extendido);

void inverter_string(char str2[], char str1[])
{
	int i,
		j;
	
	str2[0] = '\0';
	j=0;
	i=strlen(str1)-1;
	
	while(i >= 0)
	{	
		str2[j] = str1[i];
		i--;
		j++;
	}	
	
	str2[j] = '\0';
}

void init_blocos_livres(Block disco[], int total_blocos)
{
	int qnt_blocos, 
		inicio_endereco_block_livres, 
		fim_blocos,
		i,
		j;
	
	inicio_endereco_block_livres = total_blocos/10;
	fim_blocos = total_blocos/10;
	for(i=0 ; i < fim_blocos && inicio_endereco_block_livres < total_blocos; i++)
	{
		disco[i].no_lista_block.tl=-1;
		disco[i].no_lista_block.prox = i+1;
		
		
		disco[i].no_lista_block.tl++;	
		while(disco[i].no_lista_block.tl < 10 && inicio_endereco_block_livres < total_blocos)
		{
			disco[i].no_lista_block.pilha[ disco[i].no_lista_block.tl++ ] = inicio_endereco_block_livres;
			
//			disco[inicio_endereco_block_livres].tipo = 'F';
			
			inicio_endereco_block_livres++;
		}
		
	}
	
	//ULTIMO BLOCO 
	disco[i].no_lista_block.prox = -1;
}

void exibir_blocos_livres(Block disco[], int topo_blocks_free)
{
	int i,
		j;
		
	while(disco[topo_blocks_free].no_lista_block.prox != -1)
	{
		for(j = disco[topo_blocks_free].no_lista_block.tl-1 ; j > -1  ; j--)
			printf("ENDERECO => %d\n", disco[topo_blocks_free].no_lista_block.pilha[j]);
			
		topo_blocks_free++;	
	}
		
}

void pop_lista_block(Block disco[], int * topo_blocks_free, int * endereco)
{
	//TRATAR BLOCOS BAD, POIS BADBLOCKS NAO PODEM SER USADOS
	int tl;
		
	tl = 	disco[*topo_blocks_free].no_lista_block.tl;
	*endereco = disco[*topo_blocks_free].no_lista_block.pilha[tl-1];
	
	disco[*topo_blocks_free].no_lista_block.tl--;
	if(disco[*topo_blocks_free].no_lista_block.tl == 0)
		(*topo_blocks_free)++;
}


void push_lista_block(Block disco[], int * topo_blocks_free, int endereco)
{
	int	tl;
	
	if(disco[*topo_blocks_free].no_lista_block.tl == 10)
		(*topo_blocks_free)--;

		
	tl = 	++disco[*topo_blocks_free].no_lista_block.tl;
	disco[*topo_blocks_free].no_lista_block.pilha[tl] = endereco;
}

int quantidade_blocks_livres(Block disco[], int topo_blocks_free)
{
	int qntd_blocks,
		i,
		j;
	
	/*NOS BLOCOS RESERVADOS, PEGO DESDE O INICIO(ATUAL) 
	CONTANDO A PILHA INTERNA
	DELE E CONTO ATE O FINAL
	
	*/qntd_blocks=0;
	while(disco[topo_blocks_free].no_lista_block.prox != -1)
	{
		for(j = disco[topo_blocks_free].no_lista_block.tl-1 ; j > -1  ; j--)
			qntd_blocks++;
			
		topo_blocks_free++;	
	}
	
	return qntd_blocks;
}

void criar_estrutura_diretorio( Block disco[], int * topo_blocks_free, int endereco_atual, int endereco_anterior)
{	
	//ALOCA UM DIRETORIO NO DISCO
	
	disco[endereco_atual].diretorio.i_numero[0] = endereco_atual; // .
	disco[endereco_atual].diretorio.i_numero[1] = (endereco_anterior == -1) ? endereco_atual : endereco_anterior ; 	// ..
	strcpy(disco[endereco_atual].diretorio.nome_arq[0], ".");
	strcpy(disco[endereco_atual].diretorio.nome_arq[1], "..");
	disco[endereco_atual].diretorio.tl=2;	
}


void criar_estrutura_diretorio_extend( Block disco[], int * topo_blocks_free, int endereco_anterior, int endereco_atual, int endereco_block_novo)
{	
	//ALOCA UM DIRETORIO NO DISCO
	
	disco[endereco_block_novo].diretorio.i_numero[0] = endereco_atual; // .
	disco[endereco_block_novo].diretorio.i_numero[1] = endereco_anterior;  // ..	
	strcpy(disco[endereco_block_novo].diretorio.nome_arq[0], ".");
	strcpy(disco[endereco_block_novo].diretorio.nome_arq[1], "..");
	disco[endereco_block_novo].diretorio.tl=2;	
}

void criar_inode_diretorio( Block disco[], int endereco_inode, int endereco_estrutura_dir)
{
	//ALOCA UM INODE DE DIRETORIO NO DISCO
	
	strcpy(disco[endereco_inode].inode.permissoes, "drw-r--r--\0");
	strcpy(disco[endereco_inode].inode.data, __DATE__);
	strcpy(disco[endereco_inode].inode.hora, __TIME__);
	disco[endereco_inode].inode.tamanho = 2*10;
	strcpy(disco[endereco_inode].inode.nome_usuario, NOME_USUARIO_PADRAO);
	strcpy(disco[endereco_inode].inode.nome_grupo, NOME_GRUPO_PADRAO);
	disco[endereco_inode].inode.contador_link_hard = 1;
	
	disco[endereco_inode].inode.b_diretos[0] = endereco_estrutura_dir;
}

void criar_inode_block_arquivo_regular( Block disco[], int endereco_inode)
{
	//ALOCA UM INODE DE DIRETORIO NO DISCO
	
	strcpy(disco[endereco_inode].inode.permissoes, "-rw-r--r--\0");
	strcpy(disco[endereco_inode].inode.data, __DATE__);
	strcpy(disco[endereco_inode].inode.hora, __TIME__);
	disco[endereco_inode].inode.tamanho = 10;
	strcpy(disco[endereco_inode].inode.nome_usuario, NOME_USUARIO_PADRAO);
	strcpy(disco[endereco_inode].inode.nome_grupo, NOME_GRUPO_PADRAO);
	disco[endereco_inode].inode.contador_link_hard = 1;
}

void criar_diretorio_raiz(Block disco[], int * topo_blocks_free, int * inode_atual)
{
	//CRIA DIRETORIO RAIZ, INICIO DE TUDO
	
	int endereco_inode,
		endereco_estrutura_dir;
	
	pop_lista_block(disco, &*topo_blocks_free, &endereco_inode);
	pop_lista_block(disco, &*topo_blocks_free, &endereco_estrutura_dir);
	
	criar_inode_diretorio(disco, endereco_inode, endereco_estrutura_dir);
	criar_estrutura_diretorio(disco, &*topo_blocks_free, endereco_estrutura_dir, -1);
	
	*inode_atual = endereco_inode;
}

void criar_inode_arquivo( Block disco[], int * topo_blocks_free, int endereco_inode_atual, int * quantidade_blocos);

void inserir_inode_extend_simples(Block disco[], int * topo_blocks_free, int endereco_inode_atual_simples, 
								  int *quantidade_bytes, int * cont_blocks)
{
	int i,
		endereco_blocos;
	
	for(i=0 ; i < 5 && *quantidade_bytes > 0 && *cont_blocks < 159; i++)
	{
		//RETIRA ENDERECO DA LISTA DE BLOCOS;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		
		//ALOCA DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		disco[endereco_inode_atual_simples].inode_extend.b_diretos[i] = endereco_blocos;
		
		//CRIA ARQUIVO REGULAR EM DISCO
		criar_inode_block_arquivo_regular(disco, endereco_blocos);
		
		(*cont_blocks)++;
//		printf("bytes: %d, blocos: %d\n", *quantidade_bytes, *cont_blocks);
		
		if(i < 5 && *quantidade_bytes > 0 && *cont_blocks < 160)
		 (*quantidade_bytes) -= 10;
	}
	
	if(*cont_blocks == 159)
	{
		//RETIRA ENDERECO DA LISTA DE BLOCOS;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		
		//ALOCA DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		disco[endereco_inode_atual_simples].inode_extend.b_diretos[4] = endereco_blocos;
		
		//CRIA ARQUIVO REGULAR EM DISCO
		criar_inode_arquivo(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_bytes);
	}
}

void inserir_inode_extend_duplo(Block disco[], int * topo_blocks_free,
								int endereco_inode_atual_duplo, int *quantidade_block_indireto, 
								int * cont_blocks)
{
	int i,
		endereco_blocos;
	
	for(i=0 ; i < 5 && *quantidade_block_indireto > 0 ; i++)
	{
		//RETIRA ENDERECO DA LISTA DE BLOCOS;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		
		//ALOCA DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		disco[endereco_inode_atual_duplo].inode_extend.b_diretos[i] = endereco_blocos;
		
		//CRIA ARQUIVO REGULAR EM DISCO
		criar_inode_block_arquivo_regular(disco, endereco_blocos);
		
		//INSERIR OS INDIRETOS SIMPLES
		inserir_inode_extend_simples(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_block_indireto, &*cont_blocks);
		
//		(cont_blocks)++;
//		printf("bytes: %d, blocos: %d\n", *quantidade_block_indireto, *cont_blocks);
	}
}

void inserir_inode_extend_triplo(Block disco[], int * topo_blocks_free, int endereco_inode_atual_triplo, 
								 int *quantidade_block_indireto, int * cont_blocks)
{
	int i,
		endereco_blocos;
	
	for(i=0 ; i < 5 && *quantidade_block_indireto > 0 ; i++)
	{
		//RETIRA ENDERECO DA LISTA DE BLOCOS;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		
		//ALOCA DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		disco[endereco_inode_atual_triplo].inode_extend.b_diretos[i] = endereco_blocos;
		
		//CRIA ARQUIVO REGULAR EM DISCO
		criar_inode_block_arquivo_regular(disco, endereco_blocos);
		
		//INSERIR OS INDIRETOS SIMPLES
		inserir_inode_extend_duplo(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_block_indireto, &*cont_blocks);
		
//		(*cont_blocks)++;
//		printf("bytes: %d, blocos: %d\n", *quantidade_block_indireto, *cont_blocks);
	}
}

void criar_inode_arquivo( Block disco[], int * topo_blocks_free, int endereco_inode_atual, int * quantidade_bytes)
{	
	//ARQUIVO REGULAR

	int endereco_blocos,
		i,
		cont_blocks;

	disco[endereco_inode_atual].inode.tamanho = *quantidade_bytes;
	strcpy(disco[endereco_inode_atual].inode.permissoes, "-rw-r--r--\0");
	strcpy(disco[endereco_inode_atual].inode.data, __DATE__);
	strcpy(disco[endereco_inode_atual].inode.hora, __TIME__);
	strcpy(disco[endereco_inode_atual].inode.nome_usuario, NOME_USUARIO_PADRAO);
	strcpy(disco[endereco_inode_atual].inode.nome_grupo, NOME_GRUPO_PADRAO);
	disco[endereco_inode_atual].inode.contador_link_hard = 1;
	
	cont_blocks=0; //
	
//	printf("%d", (*quantidade_bytes));
	//INSERIR OS BLOCOS DIRETOS QNTD = 5
	for(i=0; i < *quantidade_bytes && i < 5 ; i++, (*quantidade_bytes) -= 10)
	{
		//PEGA BLOCK DA LISTA DE BLOCOS
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		
		//ATRIBUI ENDERECO DO BLOCO NA VARIAVEL QUE APONTA
		disco[endereco_inode_atual].inode.b_diretos[i] = endereco_blocos;
		
		//ALOCA O BLOCO REGULAR NO DISCO
		criar_inode_block_arquivo_regular(disco, endereco_blocos);
		
		cont_blocks++;
	}
	
	//BLOCOS INDIRETOS SIMPLES QNTD = 5 extend
	if(*quantidade_bytes > 0)
	{
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		disco[endereco_inode_atual].inode.b_indi_simples = endereco_blocos;
		
		inserir_inode_extend_simples(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_bytes, &cont_blocks);	
	}
	

	//BLOCOS INDIRETOS DUPLO QNTD = 5*5 extend
	if(*quantidade_bytes > 0)
	{
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		disco[endereco_inode_atual].inode.b_indi_duplo = endereco_blocos;
		
		inserir_inode_extend_duplo(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_bytes, &cont_blocks);	
	}
	
	
	//BLOCO INDIRETO TRIPLO QNTD = 5*5*5 extend
	//FALTA TRATAR O ULTIMO ENDERECO CASO O NUMERO DE BLOCOS SE PASSE de 624
	if(*quantidade_bytes > 0)
	{
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		disco[endereco_inode_atual].inode.b_indi_triplo = endereco_blocos;
		
		inserir_inode_extend_triplo(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_bytes, &cont_blocks);
	}
}

void inserir_arquivo_in_estrutura_diretorio(Block disco[], int * topo_blocks_free, 
												int endereco_inode_dir_atual, char nome_arq[], 
												int quantidade_bytes_user, int extend)
{
	//INSERIR UM ARQUIVO DENTRO DO DIRETORIO ATUAL
	int endereco_arq_novo,
		endereco_dir_atual,
		endereco_dir_novo,
		endereco_inode_dir_novo,
		endereco_dir_anterior,
		tl_dir_atual,
		endereco_inode_dir_extend,
		endereco_dir_extend;
	
	if(!extend)
		//pega endereco real da estrutura, (endereco atual eh do inode)
		endereco_dir_atual = disco[endereco_inode_dir_atual].inode.b_diretos[0];
	else
		endereco_dir_atual = endereco_inode_dir_atual;
	
	/*POS 0 = .
	  POS 1 = ..
	  POS 13 = DIRETORIO EXTENDIDO
	*/
	if(disco[endereco_dir_atual].diretorio.tl == 12)
	{
		endereco_dir_atual = disco[endereco_dir_atual].diretorio.i_numero[0];
		endereco_dir_anterior = disco[endereco_dir_atual].diretorio.i_numero[1];
		
		//retira endereco para o inode do diretorio novo;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_dir_novo);
		
		//retira endereco para o diretorio novo
//		pop_lista_block(disco, &*topo_blocks_free, &endereco_inode_dir_novo);
		
		//aloca inode do diretorio novo
//		criar_inode_diretorio(disco, endereco_inode_dir_novo, endereco_dir_novo);
		
		//aloca estrutura de diretorio do diretorio novo;
		criar_estrutura_diretorio_extend(disco, &*topo_blocks_free, endereco_dir_anterior, 
										 endereco_dir_atual, endereco_dir_novo);
		
		//POSICAO 12 RECEBE O NUMERO DO INODE DO DIRETORIO EXTENDIDO
		disco[endereco_dir_atual].diretorio.i_numero[12] = endereco_dir_novo;
		
		inserir_arquivo_in_estrutura_diretorio(disco, &*topo_blocks_free, endereco_dir_novo, nome_arq, quantidade_bytes_user, 1);									 
	}
	else if(disco[endereco_dir_atual].diretorio.tl > 12)
	{
		endereco_dir_extend = disco[endereco_dir_atual].diretorio.i_numero[12];
		inserir_arquivo_in_estrutura_diretorio(disco, &*topo_blocks_free, endereco_dir_extend, nome_arq, quantidade_bytes_user, 1);									 
	}
	else
	{
		//retira endereco para o inode do arquivo novo;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_arq_novo);
		
		//aloca inode do diretorio novo
		criar_inode_arquivo(disco, &*topo_blocks_free, endereco_arq_novo, &quantidade_bytes_user);
		
		//insere esse dir novo na estrutura atual;
		strcpy(disco[ endereco_dir_atual ].diretorio.nome_arq[ disco[endereco_dir_atual ].diretorio.tl ], nome_arq);
		disco[ endereco_dir_atual ].diretorio.i_numero[ disco[endereco_dir_atual ].diretorio.tl ] = endereco_arq_novo;
		
	}
	
	disco[endereco_dir_atual ].diretorio.tl++;
}



void inserir_diretorio_in_estrutura_diretorio(Block disco[], int * topo_blocks_free, 
												int endereco_inode_dir_atual, char nome_dir[], int extend)
{
	//INSERIR UM DIRETORIO DENTRO DO DIRETORIO ATUAL.
	
	int endereco_dir_novo,
		endereco_inode_dir_novo,
		tl_dir_atual,
		endereco_dir_anterior,
		endereco_dir_atual,
		endereco_inode_dir_extend;
	
	
	if(!extend)
		//pega endereco real da estrutura, (endereco atual eh do inode)
		endereco_dir_atual = disco[endereco_inode_dir_atual].inode.b_diretos[0];
	else
		endereco_dir_atual = endereco_inode_dir_atual;
	
	/*POS 0 = .
	  POS 1 = ..
	  POS 13 = DIRETORIO EXTENDIDO
	*/
	if(disco[endereco_dir_atual].diretorio.tl == 12)
	{
		endereco_dir_atual = disco[endereco_dir_atual].diretorio.i_numero[0];
		endereco_dir_anterior = disco[endereco_dir_atual].diretorio.i_numero[1];
		
		//retira endereco para o inode do diretorio novo;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_dir_novo);
		
		//retira endereco para o diretorio novo
//		pop_lista_block(disco, &*topo_blocks_free, &endereco_inode_dir_novo);
		
		//aloca inode do diretorio novo
		criar_inode_diretorio(disco, endereco_inode_dir_novo, endereco_dir_novo);
		
		//aloca estrutura de diretorio do diretorio novo;
		criar_estrutura_diretorio_extend(disco, &*topo_blocks_free, endereco_dir_anterior, 
										 endereco_dir_atual, endereco_dir_novo);
		
		//POSICAO 12 RECEBE O NUMERO DO INODE DO DIRETORIO EXTENDIDO
		disco[endereco_dir_atual].diretorio.i_numero[12] = endereco_dir_novo;
		
		inserir_diretorio_in_estrutura_diretorio(disco, &*topo_blocks_free, endereco_dir_novo, nome_dir, 1);					 
	}
	else if(disco[endereco_dir_atual].diretorio.tl > 12)
	{
		endereco_inode_dir_extend = disco[endereco_dir_atual].diretorio.i_numero[12];
		inserir_diretorio_in_estrutura_diretorio(disco, &*topo_blocks_free, endereco_inode_dir_extend, nome_dir, 1);		
	}
	else
	{
		//retira endereco para o inode do diretorio novo;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_dir_novo);
		
		//retira endereco para o diretorio novo
		pop_lista_block(disco, &*topo_blocks_free, &endereco_inode_dir_novo);
		
		//aloca inode do diretorio novo

		criar_inode_diretorio(disco, endereco_inode_dir_novo, endereco_dir_novo);
		
		//aloca estrutura de diretorio do diretorio novo;
		criar_estrutura_diretorio(disco, &*topo_blocks_free, endereco_dir_novo, endereco_inode_dir_atual);
		
		
		//insere esse dir novo na estrutura atual;
		strcpy(disco[ endereco_dir_atual ].diretorio.nome_arq[ disco[endereco_dir_atual ].diretorio.tl ], nome_dir);
		disco[ endereco_dir_atual ].diretorio.i_numero[ disco[endereco_dir_atual ].diretorio.tl ] = endereco_inode_dir_novo;
	}
	disco[endereco_dir_atual ].diretorio.tl++;
}


void mudar_permissao(Block disco[], int endereco_inode_atual/*, char str_arq[]*/, char str_permissao[])
{
	int i,
		j,
		pos_permissoes_inode;
	
	char retira_insere,
		 quem[3], //0 = user, 1 = group, 2 = other;
		 permissoes[3];
		 
	retira_insere='-';
	for(i=0 ; i < 3 ; i++)
		quem[i] = permissoes[i] = '-';
	
//	i_node_atual = endereco_inode_dir_atual;
//	mover_para_diretorio(disco, endereco_inode_raiz, &i_node_atual, str_arq);
	
	for(i=0 ; str_permissao[i] != '\0' && str_permissao[i] != '\n' && str_permissao[i] != '+' && str_permissao[i] != '-' ; i++);
	
	if(str_permissao[i] != '\0' && str_permissao[i] != '\n' && (str_permissao[i] == '+' || str_permissao[i] == '-'))
	{
		retira_insere = (str_permissao[i] == '+') ? '+' : '-';
		i++;
		
		//COMER ESPACO
		while(str_permissao[i] != '\0' && str_permissao[i] != '\n' && str_permissao[i] == ' ')
			i++;
		
		//QUAL ESCOPO VAI SER ALTERADO AS PERMISSOES
		while(str_permissao[i] != '\0' && str_permissao[i] != '\n' && str_permissao[i] != ' ')
		{
			if(str_permissao[i] == 'u' || str_permissao[i] == 'U')
				quem[0] = 'u';
			else if(str_permissao[i] == 'g' || str_permissao[i] == 'G')
				quem[1] = 'g';
			else if(str_permissao[i] == 'o' || str_permissao[i] == 'O')
				quem[2] = 'o';
				
			i++;	
		}
		
		//COMER ESPACO
		while(str_permissao[i] != '\0' && str_permissao[i] != '\n' && str_permissao[i] == ' ')
			i++;
		
		//QUAIS PERMISSOES
		while(str_permissao[i] != '\0' && str_permissao[i] != '\n' && str_permissao[i] != ' ')
		{
			if(str_permissao[i] == 'r' || str_permissao[i] == 'R')
				permissoes[0] = 'r';
			else if(str_permissao[i] == 'w' || str_permissao[i] == 'W')
				permissoes[1] = 'w';
			else if(str_permissao[i] == 'x' || str_permissao[i] == 'X')
				permissoes[2] = 'x';
			
			i++;	
		}
		
		if(retira_insere != '-')
		{
			pos_permissoes_inode=1;
			for(i=0 ; i < 3 ; i++)
			{
				for(j=0 ; j < 3 && quem[i] != '-' && (quem[i] == 'u' || quem[i] == 'g' || quem[i] == 'o') ; j++)
					if(permissoes[j] != '-' && (permissoes[j] == 'r' || permissoes[j] == 'w' || permissoes[j] == 'x'))
						disco[endereco_inode_atual].inode.permissoes[pos_permissoes_inode+j] = permissoes[j];	
				
				pos_permissoes_inode += 3;
			}
		}
	}
}

char split_comando_chmod(char comando[], char caminho_arquivo[], char argumento_comando[])
{
	int i,
		tl_caminho_arq,
		tl_argumento_comand;

	tl_caminho_arq=0;
	tl_argumento_comand=0;
	
	i=0;
	while(comando[i] != '\0' && comando[i] != '\n' && comando[i] == ' ')
		i++;
	
	for(; comando[i] != '\0' && comando[i] != '\n' && comando[i] != '+' && comando[i] != '-'; i++);
	
	if(comando[i] == '\0' && comando[i] == '\n')
		return 0;
	else if(comando[i] == '+' || comando[i] == '-')
	{
//		i++;
		
		while(comando[i] != '\0' && comando[i] != '\n' && comando[i] == ' ')
			i++;
		
		while(comando[i] != '\0' && comando[i] != '\n' && 
	 	 comando[i] != 'R' && comando[i] != 'r' &&
	 	 comando[i] != 'W' && comando[i] != 'w' &&
	 	 comando[i] != 'X' && comando[i] != 'x')
		{
			argumento_comando[tl_argumento_comand] = comando[i];
			tl_argumento_comand++;
			i++;
			
		}
		
		while(comando[i] != '\0' && comando[i] != '\n' && comando[i] == ' ')
			i++;
		
		if(comando[i] == '\0' && comando[i] == '\n')
			return 0;
		else
		{
			while(comando[i] != '\0' && comando[i] != '\n' && comando[i] != ' ')
			{
				argumento_comando[tl_argumento_comand] = comando[i];
				tl_argumento_comand++;
				
				i++;
			}
			argumento_comando[tl_argumento_comand] = '\0';
			
			while(comando[i] != '\0' && comando[i] != '\n' && comando[i] == ' ')
				i++;
			
			while(comando[i] != '\0' && comando[i] != '\n' && comando[i] != ' ')
			{
				caminho_arquivo[tl_caminho_arq] = comando[i];
				tl_caminho_arq++;
				i++;
				
			}
			caminho_arquivo[tl_caminho_arq]='\0';
		}		
	}	
}

void split_caminho(char caminhos[][255], int * tl_caminhos, char string_caminho[])
{
	int i,
		j,
		tl_aux_caminho,
		tl_string_caminho;
	
	char aux_caminho[255];
	
	
	//TRATA ESPACO
	i=0;
	while(string_caminho[i] != '\0' && string_caminho[i] != '\n' && string_caminho[i] != ' ')
		i++;
		
	while(string_caminho[i] != '\0' && string_caminho[i] != '\n' && string_caminho[i] == ' ')	
		i++;
	
	if(string_caminho[i] != '\0' && string_caminho[i] != '\n')
	{
		//RETIRAR COMANDO CD DA STRING
		j=0;
		while(string_caminho[i] != '\0' && string_caminho[i] != '\n' && string_caminho[i] != ' ')
		{
			aux_caminho[j] = string_caminho[i];
			j++;
			i++;
		}
		aux_caminho[j]='\0';
		strcpy(string_caminho, aux_caminho);
	}	
	
	tl_string_caminho = strlen(string_caminho);
	for(i=0 ; i < tl_string_caminho; i++)
	{
		tl_aux_caminho=0;
		
		if(string_caminho[i] != ' ')
		{
			if(string_caminho[i] == '/')
			{
				strcpy(caminhos[*tl_caminhos], "/");	
				(*tl_caminhos)++;
			}
			else
			{
				while(string_caminho[i] != '\0' && string_caminho[i] != '\n' &&
						string_caminho[i] != '/')
				{
					aux_caminho[tl_aux_caminho] = string_caminho[i];
					tl_aux_caminho++;
					i++;
				}
				aux_caminho[tl_aux_caminho] = '\0';
				
				strcpy(caminhos[*tl_caminhos], aux_caminho);	
				(*tl_caminhos)++;
				
				if(string_caminho[i] == '/')
				{
					strcpy(caminhos[*tl_caminhos], "/");	
					(*tl_caminhos)++;
				}
			}
		}
	}
}


char buscar_inode(Block disco[], int endereco_inode_dir_raiz, 
				  int *endereco_inode_atual, int *endereco_inode_dir_atual, char string_caminho[])
{
	char caminhos[100][255],
		 caminho_valido;
	
	int i_node_atual,
		i_node_atual_aux,
		i_node_dir_atual,
		endereco_dir,
		i,
		j,
		tl_caminhos,
		endereco_path_link_simb;
	

	
	//SEPARA NOMES NUMA LISTA DE NOMES
	tl_caminhos=0;	
	split_caminho(caminhos, &tl_caminhos, string_caminho);	
	
	//CAMINHO ABSOLUTO
	if(strcmp(caminhos[0], "/") == 0)
	{
		i=1;
		i_node_atual = endereco_inode_dir_raiz;
	}
		
	//CAMINHO RELATIVO
	else
	{
		i=0;
		i_node_atual = *endereco_inode_dir_atual;
	}
	
	caminho_valido=1;
	while(i < tl_caminhos && caminho_valido)
	{
		if(strcmp(caminhos[i], "/") != 0)
		{
			if(disco[i_node_atual].inode.permissoes[0] == 'd')
			{
				endereco_dir = disco[i_node_atual].inode.b_diretos[0];
			
				//procurar dentro do diretorio o nome do diretorio i
				for(j=0 ; j < disco[endereco_dir].diretorio.tl && 
					strcmp(disco[endereco_dir].diretorio.nome_arq[j], caminhos[i]) != 0; j++);
					
				if(j < disco[endereco_dir].diretorio.tl && 
				strcmp(disco[endereco_dir].diretorio.nome_arq[j], caminhos[i]) == 0)
				{
					i_node_dir_atual = endereco_dir;
					i_node_atual = disco[endereco_dir].diretorio.i_numero[j];
				}
							
				
				else
					caminho_valido=0;
			}
		}
		else if(disco[i_node_atual].inode.permissoes[0] == 'l')
		{
			//PEGO ENDERECO DO LINK SIMB
			endereco_path_link_simb = disco[i_node_atual].inode.b_diretos[0];
			
			//CHAMO A MESMA FUNCAO REC COMO CAMINHO DO LINK SIMB
			buscar_inode(disco, endereco_inode_dir_raiz, &*endereco_inode_atual, 
							&*endereco_inode_dir_atual, disco[endereco_path_link_simb].link_simb.caminho);
		}
		
		i++;
	}	

	
	if(caminho_valido)
	{
		*endereco_inode_dir_atual = i_node_dir_atual;	
		*endereco_inode_atual = i_node_atual;
		return 1;
	}
	
	return 0;
}

void inserir_link_fisico(Block disco[], int endereco_inode_origem, int endereco_inode_dir_destino, char nome_linkh_criar[])
{
	int tl_dir,
		endereco_estrutura_diretorio_atual;
	
	//INCREMENTA NUMERO DE LINKS
	disco[endereco_inode_dir_destino].inode.contador_link_hard++;
	
	//PEGA ENDERECO DA ESTRUTURA DO DIRETORIAL PARA INSERIR O NOME E O INODE NUMBER
	endereco_estrutura_diretorio_atual = disco[endereco_inode_dir_destino].inode.b_diretos[0];
	
	tl_dir = disco[endereco_estrutura_diretorio_atual].diretorio.tl;
//	printf("%s", disco[disco[ disco[endereco_dir_destino].diretorio.i_numero ].inode.b_diretos[0]].diretorio);
	//INSERE NUMERO DO INODE DE ORIGEM
	disco[endereco_estrutura_diretorio_atual].diretorio.i_numero[tl_dir] = endereco_inode_origem;
	
	//INSERE NOME DO LINK DESEJADO
	strcpy(disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[tl_dir], nome_linkh_criar);
	
	//INCREMENTA QUANTIDADE
	disco[endereco_estrutura_diretorio_atual].diretorio.tl++;
}

void split_funcao_comando_link(char caminho_origem[], char caminho_destino[], char nome_link_criar[], char cmd[])
{
	int i,
		j;
	
	char aux[255];
	
	//PEGAR NOME DO LINK FISICO
	i = strlen(cmd)-1;
	
	//TRATAR ESPACOS
	while(i >= 0 && cmd[i] == ' ')
		i--;
	
	j=0;
	while(i >= 0 && cmd[i] != ' ' && cmd[i] != '/')
	{
		aux[j] = cmd[i];
		i--;
		j++;
	}
	aux[j] = '\0';
	inverter_string(nome_link_criar, aux);
//	printf("%s", nome_link_criar);
		
	
	if(i >= 0 && strlen(nome_link_criar) > 0)
	{
		//TRATAR ESPACOS
		while(i >= 0 && cmd[i] == ' ' || (cmd[i] == '/' && cmd[i-1] != ' '))
			i--;
		
		//PEGAR DESTINO
		j=0;
		while(i >= 0 && cmd[i] != ' ')
		{
			aux[j] = cmd[i];
			i--;
			j++;
		}	
		aux[j] = '\0';
		inverter_string(caminho_destino, aux);
//		printf("%s", caminho_destino);
		
		if(i >= 0 && strlen(caminho_destino) > 0)
		{
			//TRATAR ESPACOS
			while(i >= 0 && cmd[i] == ' ' || cmd[i] == '/')
				i--;
			
			//PEGAR ORIGEM
			j=0;
			while(i >= 0 && cmd[i] != ' ')
			{
				aux[j] = cmd[i];
				i--;
				j++;
			}	
			aux[j] = '\0';
			inverter_string(caminho_origem, aux);
//			printf("%s", caminho_origem);
		}
		else
		{
			strcpy(caminho_origem, caminho_destino);
			caminho_destino[0] = '\0';
		}
			
	}
}

//fun��o -  link �h  
void link_fisico(Block disco[], int *topo_blocks_free, int endereco_inode_dir_raiz, 
					int endereco_inode_dir_atual, char comando[])
{
	//STRING COMANDO CONTEM PRIMEIRO O CAMINHO DA ORIGEM DO ARQUIVO E SEGUNDO O CAMINHO DESTINO DO LINK
	
	char caminho_origem[100],
		 caminho_destino[100],
		 nome_linkh_criar[100];
	
	int endereco_inode_atual,
		endereco_inode_origem,
		endereco_inode_dir_destino,
		endereco_inode_dir_posicao_atual;
	
	
	caminho_origem[0] = '\0';
	caminho_destino[0] = '\0';
	nome_linkh_criar[0] = '\0';
	
	endereco_inode_dir_posicao_atual = endereco_inode_dir_atual;
	
	split_funcao_comando_link(caminho_origem, caminho_destino, nome_linkh_criar, comando);
	
	
	if(strlen(caminho_origem) > 0/* && strlen(caminho_destino) > 0*/)
	{
		//BUSCAR CAMINHO DE ORIGEM
		if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_atual, &endereco_inode_dir_atual, caminho_origem))
		{
			//VERIFICA SE O CAMINHO DE ORIGEM EH UM ARQUIVO
			if(disco[endereco_inode_atual].inode.permissoes[0] == '-')
			{
				//GUARDA O ENDERECO DE ORIGEM ACHADO
				endereco_inode_origem = endereco_inode_atual;
				
				if(strlen(caminho_destino) > 0)
				{
					//BUSCAR CAMINHO DE DESTINO
					if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_atual, &endereco_inode_dir_atual, caminho_destino))
					{
						//VERIFICA SE NO DIRETORIO DO DESTINO JA NAO TEM O NOME DESEJADO
						if(!nome_igual_diretorio(disco, endereco_inode_atual, nome_linkh_criar, 0))
						{
							if(disco[endereco_inode_atual].inode.permissoes[0] == 'd')
							{
								//GUARDA O ENDERECO DO INODE DE DIRETORIO DESTINO ACHADO
								endereco_inode_dir_destino = endereco_inode_atual;
								
								inserir_link_fisico(disco, endereco_inode_origem, endereco_inode_dir_destino, nome_linkh_criar);
							}
							else
								printf("DESTINO TEM QUE SER UM ARQUIVO VALIDO");
						}
						else
							printf("JA ESSE NOME EXISTE NO DIRETORIO DE DESTINO.\n");
					}
					else
						printf("ENDERECO DO DESTINO NAO EXISTE.\n");
				}
				//NAO TEM CAMINHO DE ORIGEM, CRIAR LINK NO PROPRIO DIRETORIO ATUAL
				else
				{
					//VERIFICA SE NO DIRETORIO DO DESTINO JA NAO TEM O NOME DESEJADO
					if(!nome_igual_diretorio(disco, endereco_inode_dir_posicao_atual, nome_linkh_criar, 0))
					{
						//GUARDA O ENDERECO DO INODE DE DIRETORIO DESTINO ACHADO
						endereco_inode_dir_destino = endereco_inode_dir_posicao_atual;
						
						inserir_link_fisico(disco, endereco_inode_origem, 
							endereco_inode_dir_destino, nome_linkh_criar);
			
					}
					else
						printf("JA ESSE NOME EXISTE NO DIRETORIO DE DESTINO.\n");
				}
			}
			else
				printf("CAMINHO DE ORIGEM DEVE SER UM ARQUIVO");
		}	
	}
	else
		printf("NECESSARIO O CAMINHO DE ORIGEM.\n");
	
}

void inserir_link_simbolico(Block disco[], int *topo_blocks_free, int endereco_inode_dir_destino, char nome_linkh_criar[], char caminho_completo[])
{
	int endereco_inode_link_simb,
		endereco_estrutura_path_link,
		endereco_estrutura_diretorio_atual,
		tl_dir;
	
	//RETIRA ENDERECO PARA O INODE DO LINK
	pop_lista_block(disco, &*topo_blocks_free, &endereco_inode_link_simb);
	
	//RETIRA ENDERECO PARA O PATH DO LINK
	pop_lista_block(disco, &*topo_blocks_free, &endereco_estrutura_path_link);
	
	//PEGA ENDERECO DA ESTRUTURA DO DIRETORIAL PARA INSERIR O NOME E O INODE NUMBER
	endereco_estrutura_diretorio_atual = disco[endereco_inode_dir_destino].inode.b_diretos[0];
	
	//TL DA ESTRUTURA ONDE SERA INSERIDO O NUMERO DO INODE E NOME DO LINK
	tl_dir = disco[endereco_estrutura_diretorio_atual].diretorio.tl;
	
	//INSERE NUMERO DO INODE DE ORIGEM
	disco[endereco_estrutura_diretorio_atual].diretorio.i_numero[tl_dir] = endereco_inode_link_simb;
	
	//INSERE NOME DO LINK DESEJADO
	strcpy(disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[tl_dir], nome_linkh_criar);
	
	//INCREMENTA TL DO DIRETORIO
	disco[endereco_estrutura_diretorio_atual].diretorio.tl++;
	
	//INSERIR DADOS NO INODE DO LINK
	disco[endereco_inode_link_simb].inode.tamanho = 10;
	strcpy(disco[endereco_inode_link_simb].inode.permissoes, "lrw-r--r--\0");
	strcpy(disco[endereco_inode_link_simb].inode.data, __DATE__);
	strcpy(disco[endereco_inode_link_simb].inode.hora, __TIME__);
	strcpy(disco[endereco_inode_link_simb].inode.nome_usuario, NOME_USUARIO_PADRAO);
	strcpy(disco[endereco_inode_link_simb].inode.nome_grupo, NOME_GRUPO_PADRAO);
	disco[endereco_inode_link_simb].inode.contador_link_hard = 1;
	
	disco[endereco_inode_link_simb].inode.b_diretos[0] = endereco_estrutura_path_link;
	
	//INSERIR NOME DO PATH NA ESTRUTURA DO PATH
	strcpy(disco[endereco_estrutura_path_link].link_simb.caminho, caminho_completo);
}

//fun��o -  link �s
void link_simbolico(Block disco[], int *topo_blocks_free, int endereco_inode_dir_raiz, int endereco_inode_dir_atual, char comando[])
{
	//STRING COMANDO CONTEM PRIMEIRO O CAMINHO DA ORIGEM DO ARQUIVO E SEGUNDO O CAMINHO DESTINO DO LINK
	
	char caminho_origem[100],
		 caminho_destino[100],
		 nome_linkh_criar[100],
		 caminho_completo[100];
	
	int endereco_inode_atual,
		endereco_inode_origem,
		endereco_inode_dir_destino,
		endereco_inode_dir_posicao_atual;
	
	
	caminho_origem[0] = '\0';
	caminho_destino[0] = '\0';
	nome_linkh_criar[0] = '\0';
	
	
	if(quantidade_blocks_livres(disco, *topo_blocks_free) >= 2)
	{
		endereco_inode_dir_posicao_atual = endereco_inode_dir_atual;
		
		split_funcao_comando_link(caminho_origem, caminho_destino, nome_linkh_criar, comando);
		
		
		if(strlen(caminho_origem) > 0/* && strlen(caminho_destino) > 0*/)
		{
			if(strlen(caminho_destino) > 0)
			{
				//BUSCAR CAMINHO DE DESTINO
				if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_atual, 
									&endereco_inode_dir_atual, caminho_destino))
				{
					//VERIFICA SE NO DIRETORIO DO DESTINO JA NAO TEM O NOME DESEJADO
					if(!nome_igual_diretorio(disco, endereco_inode_atual, nome_linkh_criar, 0))
					{
						if(disco[endereco_inode_atual].inode.permissoes[0] == 'd')
						{
							//GUARDA O ENDERECO DO INODE DE DIRETORIO DESTINO ACHADO
							endereco_inode_dir_destino = endereco_inode_atual;
							
							//FAZER NOME COMPLETO
							strcat(caminho_completo, caminho_destino);
							strcat(caminho_completo, "/");
							strcat(caminho_completo, nome_linkh_criar);
							
							//INSERIR
							inserir_link_simbolico(disco, &*topo_blocks_free, endereco_inode_dir_destino, 
													nome_linkh_criar, caminho_completo);
						}
						else
							printf("DESTINO TEM QUE SER UM DIRETORIO VALIDO");
					}
					else
						printf("JA ESSE NOME EXISTE NO DIRETORIO DE DESTINO.\n");
				}
				else
					printf("ENDERECO DO DESTINO NAO EXISTE.\n");
			}
			//NAO TEM CAMINHO DE ORIGEM, CRIAR LINK NO PROPRIO DIRETORIO ATUAL
			else
			{
				//VERIFICA SE NO DIRETORIO DO DESTINO JA NAO TEM O NOME DESEJADO
				if(!nome_igual_diretorio(disco, endereco_inode_dir_posicao_atual, nome_linkh_criar, 0))
				{
					//GUARDA O ENDERECO DO INODE DE DIRETORIO DESTINO ACHADO
					endereco_inode_dir_destino = endereco_inode_dir_posicao_atual;
					
					//INSERIR
					inserir_link_simbolico(disco, &*topo_blocks_free, endereco_inode_dir_destino, nome_linkh_criar, nome_linkh_criar);
				}
				else
					printf("JA ESSE NOME EXISTE NO DIRETORIO DE DESTINO.\n");
			}	
		}
		else
			printf("NECESSARIO O CAMINHO DE ORIGEM.\n");
	}
	else
		printf("ESPACO INSUFICIENTE.\n");
}

void remover_link_fisico(Block disco[], int endereco_inode_dir_destino, char nome_linkh_criar[])
{
	int tl_dir,
		endereco_estrutura_diretorio_atual,
		i;
	
	//INCREMENTA NUMERO DE LINKS
	disco[endereco_inode_dir_destino].inode.contador_link_hard--;
	
	//PEGA ENDERECO DA ESTRUTURA DO DIRETORIAL PARA INSERIR O NOME E O INODE NUMBER
	endereco_estrutura_diretorio_atual = disco[endereco_inode_dir_destino].inode.b_diretos[0];
	
	tl_dir = disco[endereco_estrutura_diretorio_atual].diretorio.tl;
//	printf("%s", disco[disco[ disco[endereco_dir_destino].diretorio.i_numero ].inode.b_diretos[0]].diretorio);
	
	
	//INSERE NOME DO LINK DESEJADO
	for(i=0 ; i < tl_dir && 
				strcmp(disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[tl_dir], nome_linkh_criar ) != 0; i++);
	
	while(i < tl_dir-1 && 
				strcpy(disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[tl_dir], nome_linkh_criar) == 0)
	{
		strcpy(disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[i],	
			disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[i+1]);
		
		disco[endereco_estrutura_diretorio_atual].diretorio.i_numero[i] = 
			disco[endereco_estrutura_diretorio_atual].diretorio.i_numero[i+1];
		
		i++;		
	}	
	
	//DECREMENTA QUANTIDADE
	disco[endereco_estrutura_diretorio_atual].diretorio.tl--;
}

void split_funcao_comando_unlink(char caminho_destino[], char nome_link_criar[], char cmd[])
{
	int i,
		j;
	
	char aux[255];
	
	//PEGAR NOME DO LINK FISICO
	i = strlen(cmd)-1;
	
	//TRATAR ESPACOS
	while(i >= 0 && cmd[i] == ' ')
		i--;
	
	j=0;
	while(i >= 0 && cmd[i] != ' ' && cmd[i] != '/')
	{
		aux[j] = cmd[i];
		i--;
		j++;
	}
	aux[j] = '\0';
	inverter_string(nome_link_criar, aux);
//	printf("%s", nome_link_criar);
		
	
	if(i >= 0 && strlen(nome_link_criar) > 0)
	{
		//TRATAR ESPACOS
		while(i >= 0 && cmd[i] == ' ' || (cmd[i] == '/' && cmd[i-1] != ' '))
			i--;
		
		//PEGAR DESTINO
		j=0;
		while(i >= 0 && cmd[i] != ' ')
		{
			aux[j] = cmd[i];
			i--;
			j++;
		}	
		aux[j] = '\0';
		inverter_string(caminho_destino, aux);
//		printf("%s", caminho_destino);	
	}
	//else
//	{
//		strcpy(caminho_origem, caminho_destino);
//		caminho_destino[0] = '\0';
//	}
}

//fun��o unlink - h
void unlink_fisico(Block disco[], int *topo_blocks_free, int endereco_inode_dir_raiz, int endereco_inode_dir_atual, char comando[])
{
		//STRING COMANDO CONTEM PRIMEIRO O CAMINHO DA ORIGEM DO ARQUIVO E SEGUNDO O CAMINHO DESTINO DO LINK
	
	char caminho_destino[100],
		 nome_linkh_criar[100];
	
	int endereco_inode_atual,
		endereco_inode_dir_destino,
		endereco_inode_dir_posicao_atual;
	
	
	caminho_destino[0] = '\0';
	nome_linkh_criar[0] = '\0';
	
	endereco_inode_dir_posicao_atual = endereco_inode_dir_atual;
	
	split_funcao_comando_unlink(caminho_destino, nome_linkh_criar, comando);
	
	

	if(strlen(caminho_destino) > 0)
	{
		//BUSCAR CAMINHO DE DESTINO
		if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_atual, &endereco_inode_dir_atual, caminho_destino))
		{
			//VERIFICA SE  TEM O NOME DESEJADO
			if(nome_igual_diretorio(disco, endereco_inode_atual, nome_linkh_criar, 0))
			{
				if(disco[endereco_inode_atual].inode.permissoes[0] == 'd')
				{
					//GUARDA O ENDERECO DO INODE DE DIRETORIO DESTINO ACHADO
					endereco_inode_dir_destino = endereco_inode_atual;
					
					remover_link_fisico(disco, endereco_inode_dir_destino, nome_linkh_criar);
				}
				else
					printf("ENDERECO NAO CONTEM O LINK INFORMADO");
			}
			else
				printf("NOME NAO EXISTE NO DIRETORIO DE DESTINO.\n");
		}
		else
			printf("ENDERECO DO DESTINO NAO EXISTE.\n");
	}
	//NAO TEM CAMINHO DE ORIGEM, CRIAR LINK NO PROPRIO DIRETORIO ATUAL
	else
	{
		//VERIFICA SE TEM O NOME DESEJADO
		if(nome_igual_diretorio(disco, endereco_inode_dir_posicao_atual, nome_linkh_criar, 0))
		{
			//GUARDA O ENDERECO DO INODE DE DIRETORIO DESTINO ACHADO
			endereco_inode_dir_destino = endereco_inode_dir_posicao_atual;
			
			remover_link_fisico(disco, endereco_inode_dir_destino, nome_linkh_criar);
		}
		else
			printf("LINK NAO ENCONTRADO");
	}
}

void remover_link_simbolico(Block disco[], int *topo_blocks_free, int endereco_inode_dir_destino, char nome_link_deletar[])
{
	int endereco_inode_link_simb,
		endereco_estrutura_path_link,
		endereco_estrutura_diretorio_atual,
		tl_dir,
		i;
	
	endereco_inode_link_simb = 
	
	//PEGA ENDERECO DA ESTRUTURA DO DIRETORIAL PARA INSERIR O NOME E O INODE NUMBER
	endereco_estrutura_diretorio_atual = disco[endereco_inode_dir_destino].inode.b_diretos[0];
	
	//TL DA ESTRUTURA ONDE SERA INSERIDO O NUMERO DO INODE E NOME DO LINK
	tl_dir = disco[endereco_estrutura_diretorio_atual].diretorio.tl--;
	
	//PROCURAR NOME DO LINK A ESTRUTURA DE DIRETORIO ATUAL
	for(i=0 ; i < tl_dir && 
			strcmp( disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[i], nome_link_deletar) !=0 ; i++);
			
	if(strcmp( disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[i], nome_link_deletar) == 0)
	{
		endereco_inode_link_simb = disco[endereco_estrutura_diretorio_atual].diretorio.i_numero[i];
		endereco_estrutura_path_link = disco[endereco_inode_link_simb].inode.b_diretos[0];
		
		//REMANEJAR A ESTRUTURA DE DIRETORIO ATUAL
		while(i < tl_dir-1)
		{
			strcpy(disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[i], 
				disco[endereco_estrutura_diretorio_atual].diretorio.nome_arq[i+1]);
				
			disco[endereco_estrutura_diretorio_atual].diretorio.i_numero[i] = 
				disco[endereco_estrutura_diretorio_atual].diretorio.i_numero[i+1];	
		}
	}		
	
	
	//INSERE ENDERECO PARA O INODE DO LINK
	push_lista_block(disco, &*topo_blocks_free, endereco_inode_link_simb);
	
	//INSERE ENDERECO PARA O PATH DO LINK
	push_lista_block(disco, &*topo_blocks_free, endereco_estrutura_path_link);
}

//fun��o unlink - s
void unlink_simbolico(Block disco[], int *topo_blocks_free, int endereco_inode_dir_raiz, int endereco_inode_dir_atual, char comando[])
{
	//STRING COMANDO CONTEM PRIMEIRO O CAMINHO DA ORIGEM DO ARQUIVO E SEGUNDO O CAMINHO DESTINO DO LINK
	
	char caminho_origem[100],
		 caminho_destino[100],
		 nome_linkh_criar[100],
		 caminho_completo[100];
	
	int endereco_inode_atual,
		endereco_inode_origem,
		endereco_inode_dir_destino,
		endereco_inode_dir_posicao_atual;
	
	
	caminho_origem[0] = '\0';
	caminho_destino[0] = '\0';
	nome_linkh_criar[0] = '\0';
	
	

	endereco_inode_dir_posicao_atual = endereco_inode_dir_atual;
	
	split_funcao_comando_unlink(caminho_destino, nome_linkh_criar, comando);
	
	
	if(strlen(caminho_destino) > 0)
	{
		//BUSCAR CAMINHO DE DESTINO
		if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_atual, 
							&endereco_inode_dir_atual, caminho_destino))
		{
			//VERIFICA SE NO DIRETORIO DO DESTINO JA NAO TEM O NOME DESEJADO
			if(nome_igual_diretorio(disco, endereco_inode_atual, nome_linkh_criar, 0))
			{
				if(disco[endereco_inode_atual].inode.permissoes[0] == 'd')
				{
					//GUARDA O ENDERECO DO INODE DE DIRETORIO DESTINO ACHADO
					endereco_inode_dir_destino = endereco_inode_atual;
					//
//					//FAZER NOME COMPLETO
//					strcat(caminho_completo, caminho_destino);
//					strcat(caminho_completo, "/");
//					strcat(caminho_completo, nome_linkh_criar);
//					
					//INSERIR
					remover_link_simbolico(disco, &*topo_blocks_free, endereco_inode_dir_destino, nome_linkh_criar);
				}
				else
					printf("DESTINO TEM QUE SER UM DIRETORIO VALIDO");
			}
			else
				printf("NOME NAO EXISTE NO DIRETORIO DE DESTINO.\n");
		}
		else
			printf("ENDERECO DO DESTINO NAO EXISTE.\n");
	}
	//NAO TEM CAMINHO DE ORIGEM, CRIAR LINK NO PROPRIO DIRETORIO ATUAL
	else
	{
		//VERIFICA SE NO DIRETORIO DO DESTINO JA NAO TEM O NOME DESEJADO
		if(nome_igual_diretorio(disco, endereco_inode_dir_posicao_atual, nome_linkh_criar, 0))
		{
			//GUARDA O ENDERECO DO INODE DE DIRETORIO DESTINO ACHADO
			endereco_inode_dir_destino = endereco_inode_dir_posicao_atual;
			
			//INSERIR
			remover_link_simbolico(disco, &*topo_blocks_free, endereco_inode_dir_destino, 
				nome_linkh_criar);
		}
		else
			printf("NOME NAO EXISTE NO DIRETORIO DE DESTINO.\n");
	}	


}

void split_funcao_criar_diretorio(char nome_arquivo[], char caminho[], char cmd_full[])
{
	int i,
		j;
	char aux[255];
	
	nome_arquivo[0] = '\0';
	caminho[0] = '\0';
	
	//RETIRA MKDIR
	for(i=0 ; cmd_full[i] != '\0' && cmd_full[i] != '\0' && cmd_full[i] != ' '; i++);
	
	//TRATA ESPACO
	while(cmd_full[i] == ' ') 
		i++;
	
	for(j=0 ; cmd_full[i] != '\0' && cmd_full[i] != '\0'; i++, j++)
		cmd_full[j] = cmd_full[i];
	cmd_full[j] = '\0';
	
	
	//PEGA NOME DO ARQUIVO
	j=0;	
	i=strlen(cmd_full)-1;			
	
	//TRATA ESPACO
	while(cmd_full[i] == ' ') 
		i--;
	
	while(i >= 0 && cmd_full[i] != '/')
	{
		aux[j] = cmd_full[i];
		i--;
		j++;
	}
	aux[j] = '\0';
	inverter_string(nome_arquivo, aux);
	
	//PEGA CAMINHO DO DIRETORIO, SE OUVER
	if(i >= 0 && cmd_full[i] == '/')
	{
			
		//TRATA ESPACO
		while(cmd_full[i] == ' ' || cmd_full[i] == '/' ) 
			i--;
			
//		i--;
		j=0;
		while(i >= 0 && cmd_full[i] != ' ')
		{
			aux[j] = cmd_full[i];
			i--;
			j++;
		}
		aux[j] = '\0';
		inverter_string(caminho, aux);
	}	
}

char nome_igual_diretorio(Block disco[], int endereco_inode_dir_atual, char nome_arquivo[], int dir_extendido)
{
	int endereco_estrutura,
		i;
	
	if(!dir_extendido)
		endereco_estrutura = disco[endereco_inode_dir_atual].inode.b_diretos[0];
	else
		endereco_estrutura = endereco_inode_dir_atual;
	
	for(i=0 ; i < disco[endereco_estrutura].diretorio.tl && i < 12  && 
		strcmp(disco[endereco_estrutura].diretorio.nome_arq[i], nome_arquivo) != 0; i++);
		
	if(i < disco[endereco_estrutura].diretorio.tl && i < 12  && 
		strcmp(disco[endereco_estrutura].diretorio.nome_arq[i], nome_arquivo) == 0)
		return 1;
	else if(i < disco[endereco_estrutura].diretorio.tl && i == 12) //PROCURA NO DIRETORIO EXTENDIDO
		return 	nome_igual_diretorio(disco, disco[endereco_estrutura].diretorio.i_numero[12], nome_arquivo, 1);
	else
		return 0;	
}

// funcao mkdir nome dir
void criar_diretorio(Block disco[], int * topo_blocks_free,int endereco_inode_dir_raiz,
					 int endereco_inode_dir_atual, char comando[255])
{
	char nome_arquivo[255],
		 caminho[255];
	
	int endereco_inode_atual;
	
	split_funcao_criar_diretorio(nome_arquivo, caminho, comando);
	
	//VERIFICA NOME DO DIRETORIO
	if(strlen(nome_arquivo) > 0)
	{	//VERIFICA CAMINHO SE OUVER
		if(strlen(caminho) > 0)
		{
			//BUSCA ESSE CAMINHO, SE EXISTIR
			if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_atual ,&endereco_inode_dir_atual, caminho))
			{
				//VERIFICA SE O CAMINHO EH DIRETORIO
				if(disco[endereco_inode_atual].inode.permissoes[0] == 'd')
				{
					if(!nome_igual_diretorio(disco, endereco_inode_atual, nome_arquivo, 0))
					{
						// minimo para criar um diretorio, 1 block para inode outro para estrutura de dir
						if(quantidade_blocks_livres(disco, *topo_blocks_free) >= 2)
							inserir_diretorio_in_estrutura_diretorio(disco, &*topo_blocks_free, endereco_inode_atual, nome_arquivo, 0);	
					}
					else
						printf("NOME DE ARQUIVO JA EXISTE.\n");
					
				}
				else
					printf("ESPACO EM DISCO INSUFICIENTE.\n");
			}
			else
				printf("CAMINHO ESPECIFICADO NAO VALIDO.\n");		
		}
		else
		{
			
			//BUSCA ESSE CAMINHO, SE EXISTIR
			if(!nome_igual_diretorio(disco, endereco_inode_dir_atual, nome_arquivo, 0))
			{
				// minimo para criar um diretorio, 1 block para inode outro para estrutura de dir		
				if(quantidade_blocks_livres(disco, *topo_blocks_free) >= 2)
					inserir_diretorio_in_estrutura_diretorio(disco, &*topo_blocks_free, endereco_inode_dir_atual, nome_arquivo, 0);
				else
					printf("ESPACO EM DISCO INSUFICIENTE.\n");
			}
			else
				printf("NOME DE ARQUIVO JA EXISTE.\n");
		}
	}
	else
		printf("COMANDO ESTA ERRADO.\n");
}

//funcao ls
void listar_diretorio(Block disco[], int endereco_inode_dir_atual, int extendido)
{
	int endereco_struct_dir,
		i,
		j,
		tl_dir,
		endereco_struct_extend;
	
	if(extendido)
		endereco_struct_dir = endereco_inode_dir_atual;
	else
		endereco_struct_dir = disco[endereco_inode_dir_atual].inode.b_diretos[0];
	
	for(i=0 ; i < disco[endereco_struct_dir].diretorio.tl && i < 12 ; i++)
	{
		if(strcmp(disco[endereco_struct_dir].diretorio.nome_arq[i], ".") != 0 && 
			strcmp(disco[endereco_struct_dir].diretorio.nome_arq[i], "..") !=  0)
		{
			printf("%s\n", disco[endereco_struct_dir].diretorio.nome_arq[i]);
		}
		else if(!extendido)
			printf("%s\n" , disco[endereco_struct_dir].diretorio.nome_arq[i]);	
	}
		
//	textcolor(15);
	
	if(i == 12)
	{
		endereco_struct_extend = disco[endereco_struct_dir].diretorio.i_numero[i];
		listar_diretorio(disco, endereco_struct_extend, 1);
	}
		
	
//	textcolor(15);	
}

//funcao ls -l
void listar_diretorio_atributos(Block disco[], int endereco_inode_dir_atual, int extendido, int show_inode_number)
{
	int endereco_struct_dir,
		endereco_inode,
		i,
		j,
		tl_dir,
		endereco_struct_extend,
		endereco_estrutura_path_link;
	
	if(extendido)
		endereco_struct_dir = endereco_inode_dir_atual;
	else
		endereco_struct_dir = disco[endereco_inode_dir_atual].inode.b_diretos[0];
	
	for(i=0 ; i < disco[endereco_struct_dir].diretorio.tl && i < 12 ; i++)
	{
		if(strcmp(disco[endereco_struct_dir].diretorio.nome_arq[i], ".") != 0 && 
			strcmp(disco[endereco_struct_dir].diretorio.nome_arq[i], "..") !=  0)
		{	
			endereco_inode = disco[endereco_struct_dir].diretorio.i_numero[i];
			
			if(show_inode_number)
				printf("%d ", endereco_inode);	
			printf("%s ", disco[endereco_inode].inode.permissoes);
			printf("%5d ", disco[endereco_inode].inode.contador_link_hard);					 
			printf("%s ", disco[endereco_inode].inode.nome_usuario);
			printf("%s ", disco[endereco_inode].inode.nome_grupo);
			printf("%5d ", disco[endereco_inode].inode.tamanho/10);
			printf("%s ", disco[endereco_inode].inode.data);
			printf("%s ", disco[endereco_inode].inode.hora);
			//
//			//COR DIFERENTE POR TIPO DE ARQUIVO
//			if(disco[endereco_inode].inode.permissoes[0] == '-')
//				textcolor(5);
//			else if(disco[endereco_inode].inode.permissoes[0] == 'd')
//				textcolor(6);
//			else
//				textcolor(7);

			//CASO FOR LINK, MOSTRAR O PATH
			if(disco[endereco_inode].inode.permissoes[0] == 'l')
			{
				//NOME DO ARQUIVO
				printf("%s" , disco[endereco_struct_dir].diretorio.nome_arq[i]);
				
				endereco_estrutura_path_link = disco[endereco_inode].inode.b_diretos[0];
				printf(" -> %s\n", disco[endereco_estrutura_path_link].link_simb.caminho);
			}
			else
				printf("%s\n" , disco[endereco_struct_dir].diretorio.nome_arq[i]);
			
//			textcolor(15);	
													 
		}
		else if(!extendido)
			printf("%s\n" , disco[endereco_struct_dir].diretorio.nome_arq[i]);	
	}
	
	
	if(i == 12)
	{
		endereco_struct_extend = disco[endereco_struct_dir].diretorio.i_numero[i];
		listar_diretorio_atributos(disco, endereco_struct_extend, 1, show_inode_number);
	}
		
//	textcolor(15);	
}

void ls_and_lsl(Block disco[], int endereco_dir_atual, char comando[])
{
	char lsl;
	int i;
	
	lsl=-1;
	
	i=0;
	while(comando[i] != '\0' && comando[i] != '\n' &&
		  comando[i+1] != '\0' && comando[i+1] != '\n' && lsl == -1)
	{
		if(comando[i] == '-' && comando[i+1] == 'l')
			lsl=0;
		else if(comando[i+2] != '\0' && comando[i+2] != '\n' &&
				comando[i] == '-' && comando[i+1] == 'a' && comando[i+2] == 'l')
			lsl=1;		
		
		i++;	
	}
	
	if(lsl == -1) // 
		listar_diretorio(disco, endereco_dir_atual, 0);
	else if(lsl == 0)
		listar_diretorio_atributos(disco, endereco_dir_atual, 0, 0);
	else if(lsl == 1)
		listar_diretorio_atributos(disco, endereco_dir_atual, 0, 1);
		
	}

//funcao chmod 
void chmod(Block disco[], int endereco_inode_dir_raiz ,int endereco_inode_dir_atual, char comando[])
{
	char arquivo[255],
		 argumento_comando[255];
	int aux;
		 
	split_comando_chmod(comando, arquivo, argumento_comando);
	
	//BUSCAR INODE DO ARQUIVO OU DIRETORIO
	if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_dir_atual, &aux, arquivo))
		//MUDAR PERMISSAO DO ARQUIVO OU DIRETORIO
		mudar_permissao(disco, endereco_inode_dir_atual, argumento_comando);
	else
		printf("arquivo n�o encontrado");
}


char verificar_blocks_extend_simples(Block disco[], int i_numero, int * tamanho)
{
	char bad_block;	
	int i_node_aux,
		i;
	
	bad_block=0;
	
	for(i=0; i < *tamanho && i < 5 && !bad_block ; i++, (*tamanho)--)
	{
		i_node_aux = disco[i_numero].inode_extend.b_diretos[i];
		if(disco[i_node_aux].tipo == 'B')
			bad_block=1;
	}
	
	return bad_block;	
}

char verificar_blocks_extend_duplo(Block disco[], int i_numero, int * tamanho)
{
	//returna se ta badblock ou nao
	int i;
	char bad_block;
	
	for(i=0; i < *tamanho && i < 5 && !bad_block ; i++, (*tamanho)--)
	{
		if(verificar_blocks_extend_simples(disco, disco[i_numero].inode_extend.b_diretos[i], &*tamanho))
			bad_block=1;
	}
	
	return bad_block;
}



char verificar_blocks_extend_triplo(Block disco[], int i_numero, int * tamanho)
{
	//returna se ta badblock ou nao
	int i;
	char bad_block;
	
	bad_block=0;
	
	for(i=0; i < *tamanho && i < 4 && !bad_block ; i++, (*tamanho)--)
	{
		if(verificar_blocks_extend_duplo(disco, disco[i_numero].inode_extend.b_diretos[i], &*tamanho))
			bad_block=1;
	}
	
	if(*tamanho && !bad_block)
		bad_block = verificar_consistencia_arquivo( disco, disco[i_numero].inode_extend.b_diretos[4]);
		
	
	return bad_block;
}

char verificar_consistencia_arquivo(Block disco[], int i_num)
{
	int tamanho,
		i,
		i_num_aux;
		
	char bad_block;	
	
	bad_block=0;
	tamanho = disco[i_num].inode.tamanho/10;
	
	//VERIFICAR OS BLOCOS DIRETOS QNTD = 5
	for(i=0; i < tamanho && i < 5 && !bad_block ; i++, tamanho--)
		if(disco[disco[i_num].inode_extend.b_diretos[i]].tipo=='B')
			bad_block=1;
	
	//VERIFICAR BLOCOS INDIRETOS SIMPLES QNTD = 10
	if(tamanho && !bad_block)
		bad_block = verificar_blocks_extend_simples(disco, disco[i_num].inode.b_indi_simples, &tamanho);
	
	//VERIFICAR BLOCOS INDIRETOS DUPLO QNTD = 125
	if(tamanho && !bad_block)
		bad_block = verificar_blocks_extend_duplo(disco, disco[i_num].inode.b_indi_duplo, &tamanho);
	
	//VERIFICAR BLOCO INDIRETO TRIPLO QNTD = 625
	//FALTA TRATAR O ULTIMO ENDERECO CASO O NUMERO DE BLOCOS SE PASSE de 624
	if(tamanho && !bad_block)
		bad_block = verificar_blocks_extend_triplo(disco, disco[i_num].inode.b_indi_triplo, &tamanho);	
		
	return bad_block;	
}

void split_comando_vi(char caminho[], char end_split[])
{
	int i,
		j;
	
	i=0;
	while(caminho[i] != '\0' && caminho[i] != '\n' && caminho[i] != ' ')
		i++;
		
	if(caminho[i] != '\0' && caminho[i] != '\n' &&  caminho[i] == ' ')
	{
		while(caminho[i] != '\0' && caminho[i] != '\n' &&  caminho[i] == ' ')
			i++;
		
		if(caminho[i] != '\0' && caminho[i] != '\n' &&  caminho[i] != ' ')
		{
			j=0;
			while(caminho[i] != '\0' && caminho[i] != '\0'  && caminho[i] != ' ')
			{
				end_split[j] = caminho[i];
				j++;
				i++;	
			}
			end_split[j] = '\0';
		}	
	}	
}

//fun��o - vi nomeArquivo
void visualizar_arquivo_regular(Block disco[], int endereco_inode_raiz, int endereco_inode_dir_atual, char caminho[])
{
	char endereco_split[255];
	int aux;
		 
	split_comando_vi(caminho, endereco_split);
	
	//BUSCAR INODE DO ARQUIVO
	if(buscar_inode(disco, endereco_inode_raiz, &endereco_inode_dir_atual, &aux , endereco_split))
		if(disco[endereco_inode_dir_atual].inode.permissoes[0] == '-' && 
			verificar_consistencia_arquivo(disco, endereco_inode_dir_atual))
			printf("Arquivo Ok\n");
	else
		printf("Arquivo com BadBlock\n"); //MUDAR ESSAS FRASES DEPOIS 		
}

void deletar_diretorio_vazio(Block disco[], int *topo_blocks_free, int endereco_inode_dir_atual)
{
	int endereco_struct_dir,
		endereco_anterior,
		i;
	
	//PEGA ENDERECO DA STRUCT DO INODE
	endereco_struct_dir = disco[endereco_inode_dir_atual].inode.b_diretos[0];
	
	//APAGA ARQUIVO DO DIRETORIO ANTERIOR
	endereco_anterior = disco[endereco_struct_dir].diretorio.i_numero[1]; //pega inode anterior
	endereco_anterior = disco[endereco_struct_dir].inode.b_diretos[0]; //pega struct desse inode anterior
	
	//ACHA O INODE NUMBER NO DIRETORIO ANTERIROR PARA APAGAR O DIRETORIO EM QUESTAO
	for(i=0 ; i < disco[endereco_anterior].diretorio.tl && 
		disco[endereco_anterior].diretorio.i_numero[i] != endereco_inode_dir_atual ; i++);
		
	//REMANEJA ARQUIVO RETIRADO DO DIRETORIO ANTERIOR	
	while(i < disco[endereco_anterior].diretorio.tl-1)
	{
		strcpy(disco[endereco_anterior].diretorio.nome_arq[i], disco[endereco_anterior].diretorio.nome_arq[i+1]);
		disco[endereco_anterior].diretorio.i_numero[i] = disco[endereco_anterior].diretorio.i_numero[i+1];
		
		i++;
	}	
	
	//DEVOLVE ESTRUTURA NUMBER
	push_lista_block(disco, &*topo_blocks_free, endereco_struct_dir);
	
	//DEVOLVE INODE NUMBER 
	push_lista_block(disco, &*topo_blocks_free, endereco_inode_dir_atual);
}

//fun��o rmdir
void rmdir(Block disco[], int * topo_blocks_free, int endereco_inode_dir_raiz, int endereco_inode_dir_atual, char str_comando[])
{
	char nome_arquivo[255],
		 caminho[255];
	int endereco_struct_dir,
		aux;
	
	split_funcao_criar_diretorio(nome_arquivo, caminho, str_comando);
	
	//VERIFICA NOME DO DIRETORIO
	if(strlen(nome_arquivo) > 0)
	{	//VERIFICA CAMINHO SE OUVER
		if(strlen(caminho) > 0)
		{
			//BUSCA ESSE CAMINHO, SE EXISTIR
			if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_dir_atual, &aux ,caminho))
			{			
				//VERIFICA SE O CAMINHO EH DIRETORIO
				//PRECISA CHECAR A PERMISSAO
				if(disco[endereco_inode_dir_atual].inode.permissoes[0] == 'd')
				{
					endereco_struct_dir = disco[endereco_inode_dir_atual].inode.b_diretos[0];
					if(disco[endereco_struct_dir].diretorio.tl == 2)
						deletar_diretorio_vazio(disco, &*topo_blocks_free, endereco_inode_dir_atual);
					else
						printf("DIRETORIO NAO ESTA VAZIO.\n");
				}
				else
					printf("IMPOSSIVEL APAGAR, NAO DIRETORIO.\n");
			}
			else
				printf("CAMINHO ESPECIFICADO NAO VALIDO.");		
		}
		else
		{
			//BUSCA ESSE CAMINHO, SE EXISTIR
			if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_dir_atual, &aux ,caminho))
			{
				//VERIFICA SE O CAMINHO EH DIRETORIO
				//PRECISA CHEGAR A PERMISSAO
				if(disco[endereco_inode_dir_atual].inode.permissoes[0] == 'd')
				{
					endereco_struct_dir = disco[endereco_inode_dir_atual].inode.b_diretos[0];
					if(disco[endereco_struct_dir].diretorio.tl == 2)
						deletar_diretorio_vazio(disco, &*topo_blocks_free, endereco_inode_dir_atual);
					else
						printf("DIRETORIO NAO ESTA VAZIO.\n");
				}
				else
					printf("IMPOSSIVEL APAGAR, NAO DIRETORIO.\n");
			}
			else
				printf("CAMINHO ESPECIFICADO NAO VALIDO.");	
		}
	}
	else
		printf("COMANDO ESTA ERRADO.");
}




void remover_inode_extend_simples(Block disco[], int * topo_blocks_free, int endereco_inode_atual_simples, 
								  int *quantidade_blocos, int * cont_blocks)
{
	int i,
		endereco_blocos;
	
	for(i=0 ; i < 5 && *quantidade_blocos > 0 && *cont_blocks < 159; i++)
	{
		//ALOCA DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		endereco_blocos = disco[endereco_inode_atual_simples].inode_extend.b_diretos[i];
		disco[endereco_inode_atual_simples].inode_extend.b_diretos[i] = 0;
		
		//INSERE BLOCO FREE DE VOLTA	
		push_lista_block(disco, &*topo_blocks_free, endereco_blocos);
		
		(*cont_blocks)++;
		
		if(i < 5 && *quantidade_blocos > 0 && *cont_blocks < 159)
		 (*quantidade_blocos)--;
	}
	
	if(*cont_blocks == 159)
	{		
		//ALOCA DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		endereco_blocos = disco[endereco_inode_atual_simples].inode_extend.b_diretos[4];
		
		//CRIA ARQUIVO REGULAR EM DISCO
		deletar_arquivo(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_blocos);
		
		//INSERE BLOCO FREE DE VOLTA	
		push_lista_block(disco, &*topo_blocks_free, endereco_blocos);
	}
}

void remover_inode_extend_duplo(Block disco[], int * topo_blocks_free,
								int endereco_inode_atual_duplo, int *quantidade_block_indireto, 
								int * cont_blocks)
{
	int i,
		endereco_blocos;
	
	for(i=0 ; i < 5 && *quantidade_block_indireto > 0 ; i++)
	{
		//ALOCA DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		endereco_blocos = disco[endereco_inode_atual_duplo].inode_extend.b_diretos[i];
		
		//INSERIR OS INDIRETOS SIMPLES
		remover_inode_extend_simples(disco, &*topo_blocks_free, endereco_blocos, 
									 &*quantidade_block_indireto, &*cont_blocks);
		
		//INSERE BLOCO FREE DE VOLTA	
		push_lista_block(disco, &*topo_blocks_free, endereco_blocos);
		disco[endereco_inode_atual_duplo].inode_extend.b_diretos[i] = 0;
		
	}
}

void remover_inode_extend_triplo(Block disco[], int * topo_blocks_free, int endereco_inode_atual_triplo, 
								 int *quantidade_block_indireto, int * cont_blocks)
{
	int i,
		endereco_blocos;
	
	for(i=0 ; i < 5 && *quantidade_block_indireto > 0 ; i++)
	{
		//PEGA ENDERECO DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		endereco_blocos = disco[endereco_inode_atual_triplo].inode_extend.b_diretos[i];
		
		//INSERIR OS INDIRETOS SIMPLES
		remover_inode_extend_duplo(disco, &*topo_blocks_free, endereco_blocos, 
									 &*quantidade_block_indireto, &*cont_blocks);
		
		//INSERE BLOCO FREE DE VOLTA	
		push_lista_block(disco, &*topo_blocks_free, endereco_blocos);
		disco[endereco_inode_atual_triplo].inode_extend.b_diretos[i] = 0;
		
//		(*cont_blocks)++;
//		printf("bytes: %d, blocos: %d\n", *quantidade_block_indireto, *cont_blocks);
	}
}

void deletar_arquivo(Block disco[], int * topo_blocks_free, int endereco_inode_atual, int *quantidade_blocos)
{
	int endereco_blocos,
		cont_blocks,
		i,
		num_int,
		resto,
		quantidade_bytes;
		
	float div;	
	
	if(*quantidade_blocos == -1)
	{
//		*quantidade_blocos = disco[endereco_inode_atual].inode.tamanho / 10;
		quantidade_bytes = disco[endereco_inode_atual].inode.tamanho;
	
		//TENHO 103 BYTES
		num_int = (int) quantidade_bytes / 10; //PARTE INTEIRA = 10
		div = (float) quantidade_bytes / 10; //PARTE FLOAT 10.3
		resto = (div - num_int) * 10; //(10.3 - 10) = 0.3 => 0.3 * 10 = 30
		
		if(resto > 0)
			*quantidade_blocos = (quantidade_bytes / 10)+1; //(103/10)+1 = 11 INTEIRO
		else	
			*quantidade_blocos = (quantidade_bytes / 10); //(103/10) = 10 INTEIRO
	}
		

	cont_blocks = 0;
	
	//REMOVER OS BLOCOS DIRETOS QNTD = 5
	for(i=0 ; i < 5 && i < *quantidade_blocos ; i++, (*quantidade_blocos)--)
	{
		
		printf("qntd blocos: %d, i=%d\n", (*quantidade_blocos), i);
		
		//PEGA ENDERECO DO BLOCO NA VARIAVEL QUE APONTA
		endereco_blocos = disco[endereco_inode_atual].inode.b_diretos[i];
		disco[endereco_inode_atual].inode.b_diretos[i] = 0;
		
		//ENVIA PARA BLOCOS LIVRES
		push_lista_block(disco, &*topo_blocks_free, endereco_blocos);
		
		cont_blocks++;
		
		
		printf("qntd blocos: %d, i=%d\n", (*quantidade_blocos), i);
	}
	
	//BLOCOS INDIRETOS SIMPLES QNTD = 5 extend
	if(*quantidade_blocos > 0)
	{
		endereco_blocos = disco[endereco_inode_atual].inode.b_indi_simples;
		remover_inode_extend_simples(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_blocos, &cont_blocks);	
		push_lista_block(disco, &*topo_blocks_free, endereco_blocos);
		disco[endereco_inode_atual].inode.b_indi_simples = 0;
	}

	//BLOCOS INDIRETOS DUPLO QNTD = 5*5 extend
	if(*quantidade_blocos > 0)
	{
		endereco_blocos = disco[endereco_inode_atual].inode.b_indi_duplo;
		remover_inode_extend_duplo(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_blocos, &cont_blocks);	
		push_lista_block(disco, &*topo_blocks_free, endereco_blocos);
		disco[endereco_inode_atual].inode.b_indi_duplo = 0;
	}
	
	
	//BLOCO INDIRETO TRIPLO QNTD = 5*5*5 extend
	//FALTA TRATAR O ULTIMO ENDERECO CASO O NUMERO DE BLOCOS SE PASSE de 624
	if(*quantidade_blocos > 0)
	{
		endereco_blocos = disco[endereco_inode_atual].inode.b_indi_triplo;
		remover_inode_extend_triplo(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_blocos, &cont_blocks);	
		push_lista_block(disco, &*topo_blocks_free, endereco_blocos);
		disco[endereco_inode_atual].inode.b_indi_triplo = 0;
	}
}


void retirar_arquivo_da_estrutua_diretorio(Block disco[], int endereco_inode_dir, int endereco_inode_arquivo)
{
	int i,
		j;
	
	//PROCURA NUMERO DO INODE DO ARQUIVO
	for(i=0 ; i < disco[endereco_inode_dir].diretorio.tl && 
		disco[endereco_inode_dir].diretorio.i_numero[i] != endereco_inode_arquivo; i++);
	
	//SE ACHOU REMANEJA OS ELEMENTOS ATE TL-1	
	if(i < disco[endereco_inode_dir].diretorio.tl && 
		disco[endereco_inode_dir].diretorio.i_numero[i] == endereco_inode_arquivo)
	{
		while(i < disco[endereco_inode_dir].diretorio.tl-1)
		{
			printf("1:%s 2:%s\n\n", disco[endereco_inode_dir].diretorio.nome_arq[i], disco[endereco_inode_dir].diretorio.nome_arq[i+1]);
			strcpy(disco[endereco_inode_dir].diretorio.nome_arq[i], disco[endereco_inode_dir].diretorio.nome_arq[i+1]);	
			disco[endereco_inode_dir].diretorio.i_numero[i] = disco[endereco_inode_dir].diretorio.i_numero[i+1];
			i++;
		}
		disco[endereco_inode_dir].diretorio.tl--;
	}
}

//fun��o - rm
void rm(Block disco[], int *topo_blocks_free, int endereco_inode_dir_raiz, int endereco_inode_dir_atual, char str_comando[])
{
	char nome_arquivo[255],
		 caminho[255];
		 
	int endereco_inode,
		quantidade_blocos,
		aux;
	
	split_funcao_criar_diretorio(nome_arquivo, caminho, str_comando);
	
	
	//VERIFICA NOME DO DIRETORIO
	if(strlen(nome_arquivo) > 0)
	{	//VERIFICA CAMINHO SE OUVER
		if(strlen(caminho) > 0)
		{
			strcat(caminho, "/\0");
			strcat(caminho, nome_arquivo);
			//BUSCA ESSE CAMINHO, SE EXISTIR
			if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode, &endereco_inode_dir_atual, caminho))
			{			
				//VERIFICA SE O CAMINHO EH DIRETORIO
				//PRECISA CHEGAR A PERMISSAO
				if(disco[endereco_inode].inode.permissoes[0] == '-')
				{
//					endereco_inode = disco[endereco_inode_dir_atual].inode.b_diretos[0];
					
					quantidade_blocos = -1; //VARIAVEL USADA NA RECURSAO
					deletar_arquivo(disco, &*topo_blocks_free, endereco_inode, &quantidade_blocos);
					
					retirar_arquivo_da_estrutua_diretorio(disco, endereco_inode_dir_atual, endereco_inode);
				}
				else
					printf("IMPOSSIVEL APAGAR, NAO EXISTE O ARQUIVO.\n");
			}
			else
				printf("CAMINHO ESPECIFICADO NAO VALIDO.");		
		}
		else
		{
			//BUSCA ESSE CAMINHO, SE EXISTIR
			if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode, &endereco_inode_dir_atual, nome_arquivo))
			{			
				//VERIFICA SE O CAMINHO EH DIRETORIO
				//PRECISA CHEGAR A PERMISSAO
				if(disco[endereco_inode].inode.permissoes[0] == '-')
				{
//					endereco_inode = disco[endereco_inode_dir_atual].inode.b_diretos[0];
					
					quantidade_blocos = -1; //VARIAVEL USADA NA RECURSAO
					deletar_arquivo(disco, &*topo_blocks_free, endereco_inode, &quantidade_blocos);
					
					retirar_arquivo_da_estrutua_diretorio(disco, endereco_inode_dir_atual, endereco_inode);
				}
				else
					printf("IMPOSSIVEL APAGAR, NAO EXISTE O ARQUIVO.\n");
			}
			else
				printf("CAMINHO ESPECIFICADO NAO VALIDO.");		
		}
	}
	else
		printf("COMANDO ESTA ERRADO.");
}


//funcao cd
char mover_para_diretorio(Block disco[], int endereco_inode_raiz ,int * endereco_inode_dir_atual,char string_caminho[])
{
	char caminhos[100][255],
		 dir_valido;
	
	int i_node_atual,
		i_node_atual_aux,
		dir_atual,
		i,
		j,
		tl_caminhos,
		endereco_path_link_simb;
	
	//SEPARA NOMES NUMA LISTA DE NOMES
	tl_caminhos=0;	
	split_caminho(caminhos, &tl_caminhos, string_caminho);	
	
	//CAMINHO ABSOLUTO
	if(strcmp(caminhos[0], "/") == 0)
	{
		i=1;
		i_node_atual = endereco_inode_raiz;
	}
		
	//CAMINHO RELATIVO
	else
	{
		i=0;
		i_node_atual = *endereco_inode_dir_atual;
	}
	//PROCESSO DE BUSCA
	dir_valido=1;
	while(i < tl_caminhos && dir_valido)
	{
//			printf("%s\n", caminhos[i]);
		if(strcmp(caminhos[i], "/") != 0)
		{
			dir_atual = disco[i_node_atual].inode.b_diretos[0];
			
			//procurar dentro do diretorio o nome do diretorio i
			for(j=0 ; j <= disco[dir_atual].diretorio.tl && 
				strcmp(disco[dir_atual].diretorio.nome_arq[j], caminhos[i]) != 0; j++);
				
			if(j <= disco[dir_atual].diretorio.tl && strcmp(disco[dir_atual].diretorio.nome_arq[j], caminhos[i]) == 0)
			{
				//verifica se realmente o nome achado eh um diretorio, olhando no inode do diretorio
				i_node_atual_aux = disco[dir_atual].diretorio.i_numero[j];
				
				//DIRETORIO?
				if(disco[i_node_atual_aux].inode.permissoes[0] == 'd')
					i_node_atual = i_node_atual_aux;
				
				//LINK SIMBOLICO?
				else if(disco[i_node_atual_aux].inode.permissoes[0] == 'l')
				{
					//PEGO ENDERECO DO LINK SIMB
					endereco_path_link_simb = disco[i_node_atual_aux].inode.b_diretos[0];
//					i_node_atual;
					
					//CHAMO A MESMA FUNCAO REC COMO CAMINHO DO LINK SIMB
					dir_valido = mover_para_diretorio(disco, endereco_inode_raiz, &i_node_atual, 
														disco[endereco_path_link_simb].link_simb.caminho);
				}
				else
					dir_valido=0;
			}
				
			else
				dir_valido=0;
		}
		
		i++;
	}	

	
	if(dir_valido)
	{
		*endereco_inode_dir_atual = i_node_atual;
		return 1;
	}
	
	return 0;
}

void split_funcao_tornar_block_bad(int * num_block, char cmd_full[])
{
	int i,
		j;
	char aux[100],
		 aux2[100];
	
	//RETIRA BAD
	for(i=0 ; cmd_full[i] != '\0' && cmd_full[i] != '\0' && cmd_full[i] != ' '; i++);
	
	if(cmd_full[i] == ' ')
		i++;
	
	for(j=0 ; cmd_full[i] != '\0' && cmd_full[i] != '\0'; i++, j++)
		cmd_full[j] = cmd_full[i];
	cmd_full[j] = '\0';
	
	
	i=strlen(cmd_full)-1;
	j=0;	
	while(i >= 0 && cmd_full[i] != ' ')
	{
		aux[j] = cmd_full[i];
		i--;
		j++;
	}
	aux[j] = '\0';
	
	if(strlen(aux) > 0)
	{	
		inverter_string(aux2, aux);
		*num_block = atoi(aux2);
	}	
}

void tornar_badblock(Block disco[], int num_block)
{
	disco[num_block].tipo = 'B';
}

char is_number(char str[]) 
{
	int i;
	
	i=0;
    while(str[i] != '\0')
    {
        if(str[i] < '0' || str[i] > '9')
            return 0;
        i++;
    }
    return 1;
}

//fun��o bad
void bad(Block disco[], int tf_disco, int endereco_inode_raiz, int endereco_inode_dir_atual, char commando_inteiro[])
{
	int num_block;
	
	num_block = -1;
	
	split_funcao_tornar_block_bad(&num_block, commando_inteiro);
	
	if(num_block < tf_disco)
		tornar_badblock(disco, endereco_inode_dir_atual);	
	else
		printf("BLOCO NAO EXISTE");
}
//
//void split_comando_touch1(char nome_arquivo[], int *qntd_blocos, char cmd[])
//{
//	int i,
//		j;
//	char number[5];
//	
//	number[0]='\0';
//	//splita comando touch
//	i=0;
//	while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] != ' ' )
//		i++;
//	
//	while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] == ' ' )
//		i++;
//	
//	//splita nome_arquivo
//	j=0;
//	while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] != ' ' )
//	{
//		nome_arquivo[j] = cmd[i];
//		i++;
//		j++;
//	}
//	nome_arquivo[j] = '\0';
//	
//	while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] == ' ' )
//		i++;
//	
//	//splita qntd bytes
//	j=0;
//	while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] != ' ' )
//	{
//		number[j] = cmd[i];
//		i++;
//		j++;
//	}
//	number[j] = '\0';
//	
//	if(strlen(number) > 0)
//		*qntd_blocos = atoi(number);	
//}

void split_comando_touch(char nome_arquivo[], char caminho[], int *qntd_bytes, char cmd[])
{
	int i,
		j;
	char number[7],
		 aux[255];
	
	number[0]='\0';
	//splita comando touch
	i=0;
	
	//TRATA ESPACO
	while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] == ' ' )
		i++;
	
	while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] != ' ' )
		i++;
	
	//TRATA ESPACO
	while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] == ' ' )
		i++;
		
	j=0;
	while(cmd[i] != '\0' && cmd[i] != '\n')
	{
		cmd[j] = cmd[i];
		i++;
		j++;
	}
	cmd[j] = '\0';
	
	//PEGA NUMBERO DE BYTES
	i = strlen(cmd)-1;
	j=0;
	
	//TRATA ESPACO
	while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] == ' ' )
		i--;
	
	while(i >= 0 && cmd[i] != '/' && cmd[i] != ' ')
	{
		aux[j] = cmd[i];
		i--;
		j++;	
	}	
	aux[j] = '\0';
	inverter_string(number, aux);
	
	if(strlen(number) > 0)
	{
		if(is_number(number))
			*qntd_bytes = atoi(number);	
		
		//TRATA ESPACO
		while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] == ' ' )
			i--;
		
		//PEGA NOME DO ARQUIVO
		j=0;
		while(i >= 0 && cmd[i] != '/' && cmd[i] != ' ')
		{
			aux[j] = cmd[i];
			i--;
			j++;	
		}	
		aux[j] = '\0';
		inverter_string(nome_arquivo, aux);
		
		if(i >= 0)
		{
			
			//TRATA ESPACO
			while(cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] == ' ' || cmd[i] == '/')
				i--;
			
			//PEGA CAMINHO DO DIRETORIO
			j=0;
			while(i >= 0 && cmd[i] != ' ')
			{
				aux[j] = cmd[i];
				i--;
				j++;	
			}	
			aux[j] = '\0';
			inverter_string(caminho, aux);
		}
	}
}

//fun��o touch
void touch( Block disco[], int * topo_blocks_free, int endereco_inode_dir_raiz, 
			int inode_endereco_dir_atual, char comando[255])
{
	char nome_arquivo[255],
		 caminho[255];
		 
	int quantidade_bytes_user,
		qntd_blocos_livres,
		num_int,
		qnt_blocos,
		resto,
		endereco_inode_atual;
	
	float div;
			
	nome_arquivo[0] = '\0';
	caminho[0] = '\0';
	quantidade_bytes_user = -1;
	
	split_comando_touch(nome_arquivo, caminho, &quantidade_bytes_user, comando);
	
	if(strlen(nome_arquivo) > 0)
	{
		if(quantidade_bytes_user > -1)
		{
			//TENHO 103 BYTES
			num_int = (int) quantidade_bytes_user / 10; //PARTE INTEIRA = 10
			div = (float) quantidade_bytes_user / 10; //PARTE FLOAT 10.3
			resto = (div - num_int) * 10; //(10.3 - 10) = 0.3 => 0.3 * 10 = 30
			
			if(resto > 0)
				qnt_blocos = (quantidade_bytes_user / 10)+1; //(103/10)+1 = 11 INTEIRO
 			else	
				qnt_blocos = (quantidade_bytes_user / 10); //(103/10) = 10 INTEIRO
			
			//minimo para criar um arquivo, 1 bloco para inode	
			qntd_blocos_livres = quantidade_blocks_livres(disco, *topo_blocks_free);
			if(qntd_blocos_livres >=  qnt_blocos + 1) 
			{
				if(strlen(caminho) > 0)
				{
					//BUSCA CAMINHO DO DIRETORIO
					if(buscar_inode(disco, endereco_inode_dir_raiz, &endereco_inode_atual, &inode_endereco_dir_atual, caminho))
					{
						//VERIFICA SE O INODE ACHADO EH UM DIRETORIO
						if(disco[endereco_inode_atual].inode.permissoes[0] == 'd')
						{
							//VERIFICA NOME IGUAL NO DIRETORIO
	/*CODIGO HADUKEN*/		if(!nome_igual_diretorio(disco, endereco_inode_atual, nome_arquivo, 0))
								inserir_arquivo_in_estrutura_diretorio(disco, &*topo_blocks_free, endereco_inode_atual, 
															   			nome_arquivo, quantidade_bytes_user, 0);
							else
								printf("NOME JA EXISTE NESSE DIRETORIO.\n");
						}
							
						else
							printf("CAMINHO TEM QUE SER UM DIRETORIO.\n");							   			
					}
					else
						printf("CAMINHO NAO EXISTE.\n");
				}
				else
				{
					//VERIFICA NOME IGUAL NO DIRETORIO
					if(!nome_igual_diretorio(disco, inode_endereco_dir_atual, nome_arquivo, 0))
								inserir_arquivo_in_estrutura_diretorio(disco, &*topo_blocks_free, inode_endereco_dir_atual, 
																		nome_arquivo, quantidade_bytes_user, 0);
					else
						printf("NOME JA EXISTE NESSE DIRETORIO.\n");
				}
			}
			else
				printf("ESPACO EM DISCO INSUFICIENTE.\n");
		}
		else
			printf("Digite uma quantidade de bytes para o touch\n");
	}
	else
		printf("Digite corretamente o arquivo\n");
}

void links(Block disco[], int * topo_blocks_free, int endereco_inode_dir_raiz, int endereco_inode_dir_atual, char comando[])
{	
	/*
		tipo_link = 0 => link fisico
		tipo_link = 1 => link_simbolico
	
	
		link_unlink = 1 =>link
		link_unlink = 0 =>unlink
	*/
	
	
	char tipo_link,
		 link_unlink,
		 tipo_comando[7];
	
	int i,
		j,
		tl_tp_comando;
	
	tipo_link = -1;
	link_unlink = -1;
	tl_tp_comando = 0;
	tipo_comando[0] = '\0';
	
	i=0;
	//TRATAR ESPACOS
	while(comando[i] != '\0' && comando[i] == ' ')
		i++;
	
	//PEGAR PARTE DA STRING QUE ESPECIFICA QUE TIPO DE LINK EH
	while(comando[i] != '\0' && tipo_link == -1)
	{
		if(comando[i+1] != '\0' && comando[i+1] != '\n' &&
			comando[i] == '-' && comando[i+1] == 'h')
				tipo_link = 0;
				
		else if(comando[i+1] != '\0' && comando[i+1] != '\n' &&
			comando[i] == '-'  && comando[i+1] == 's')
				tipo_link = 1;		
				
		if(comando[i] != ' ' && link_unlink == -1)
		{
			tipo_comando[tl_tp_comando] = comando[i];
			tl_tp_comando++;
		}
		
		if(strcmp(tipo_comando, "link") == 0)
		{
			link_unlink = 1;
			tipo_comando[tl_tp_comando] = '\0';
			tl_tp_comando++;
		}
		else if(strcmp(tipo_comando, "unlink") == 0)
		{
			link_unlink = 0;
			tipo_comando[tl_tp_comando] = '\0';
			tl_tp_comando++;
				
		}
		
		i++;
	}
	
	if(tipo_link != -1)
	{
		//RETIRAR COMANDO DEIXAR SOMENTE CAMINHOS DO LINK
		
		//TRATAR ESPACO E LUGAR PARADO NA STRING "H", "S" E "-"
		while(comando[i] == ' ' || comando[i] == '-' || comando[i] == 'h' || comando[i] == 's')
			i++;
		
		//AGORA SIM, DEIXAR A STRING COMANDO SOMENTE COM OS CAMINHOS.	
		if(comando[i] != '\0' && comando[i] != '\n')
		{
			j=0;
			while(comando[i] != '\0' && comando[i] != '\n')
			{
				comando[j] = comando[i];
				i++;
				j++;	
			}
			comando[j] = '\0';
			
			
			if(link_unlink == 1)
			{
				//CHAMAR O METODO CORRETO PARA FAZER O LINK
				if(tipo_link == 0)
					link_fisico(disco, &*topo_blocks_free, endereco_inode_dir_raiz, endereco_inode_dir_atual, comando);
				else if(tipo_link == 1)
					link_simbolico(disco, &*topo_blocks_free, endereco_inode_dir_raiz, endereco_inode_dir_atual, comando);
				else
					printf("COMANDO DE LINK INVALIDO.\n");
			}
			else if(link_unlink == 0)
			{
				if(tipo_link == 0)
					unlink_fisico(disco, &*topo_blocks_free, endereco_inode_dir_raiz, endereco_inode_dir_atual, comando);
				else if(tipo_link == 1)
					unlink_simbolico(disco, &*topo_blocks_free, endereco_inode_dir_raiz, endereco_inode_dir_atual, comando);
				else
					printf("COMANDO DE LINK INVALIDO.\n");
			}
			else
				printf("COMANDO DE LINK INVALIDO.\n");
			
		}
		else
			printf("NECESSARIO ESPECIFICAR OS CAMINHOS CORRETAMENTE.\n");
			
	}
	else
		printf("NECESSARIO FORNECER O TIPO DO LINK.\n");	
}

void df(Block disco[], int endereco_inode_dir_raiz, int endereco_inode_dir_atual)
{
	
}

void split_comando_relatorios(char cmd[], int * op)
{
	int i,
		j;
			
	char op_number[255],
		 aux[255];
	
	op_number[0] = '\0';
	
	i = strlen(cmd)-1;
	
	//TRATAR ESPACOS
	while(i >= 0 && cmd[i] == ' ')
		i--;
	
	i=0;
	while(i >= 0 && cmd[i] != ' ')
	{
		aux[j] = cmd[i];
		i--;
		j++;
	}
	aux[j] = '\0';
	
	if(is_number(aux))
	{
		inverter_string(op_number, aux);
		*op = atoi(op_number);
	}
	else
		*op = -1;
}

void mostrar_relatorios(Block disco[], int *topo_blocks_free, int endereco_inode_dir_raiz, int *endereco_inode_dir_atual, char comando[])
{
	int op;
	
	split_comando_relatorios(comando, &op);
	
	
	//APENAS MOSTRAR AS OPCOES DE RELATORIOS
	if(op == -1)
	{
		textcolor(3);
		printf("\t**** RELATORIOS ****\n\n");
		
		
		textcolor(1);
		printf("[ 1 ] - O numero de blocos ocupados por um arquivo escolhido por usuario.\n");
		printf("[ 2 ] - O tamanho (em blocos) do maior arquivo que ainda pode ser criado nesse disco.\n");
		printf("[ 3 ] - Quais arquivos est�o �ntegros e quais est�o corrompidos por blocos defeituosos (badblocks).\n");
		printf("[ 4 ] - Apresentar quantos  blocos do disco est�o perdidose quais sao eles,\
				ou seja,  nao sao  usados  por arquivose nem estao marcados como livres ou defeituosos.\
				Nao esque�a de apresentar o espa�o de disco perdido em bytes ocupados por um arquivo escolhido por usuario.\n");
		printf("[ 5 ] - Imprima todos os blocos em seu estado atual, igual a figura b.\n");
		printf("[ 6 ] - Visualize os arquivos  e  diret�rios  alocados,  apresentando  o  nome  e  seus  n�meros  do  bloco\
				correspondentes  identificando otipo.Esta  visualiza��o  deve  ser algo  semelhante  ao  Windows explore.\
				Nao precisa ser grafico\n");
		printf("[ 7 ] - Visualizar a arvore de diret�rio igual a figura D, apresentado as estruturas com seus atribuidos,\
				nomes e numeros\n");
		printf("[ 8 ] - Visualizar os links simbolicos e fisicos criados, detalhando as entradas de diret�rio, inodes e numeros\n");
		
		printf("\tEscolha uma opcao, exemplo: relatorio 2\n\n");
		
		textcolor(15);
	}
	else
	{
		switch(op)
		{
			case '1':
//				relatorio1();
				break;
			
			case '2':
//				relatorio2();
				break;
			
			case '3':
//				relatorio3();
				break;
			
			case '4':
//				relatorio4();
				break;
			
			case '5':
//				relatorio5();
				break;
			
			case '6':
//				relatorio6();
				break;
			
			case '7':
//				relatorio7();
				break;
			
			case '8':
//				relatorio8();
				break;
					
		}
	}
}

void init_str(char str[], int tam)
{
	int i;
	
	for(i=0 ; i < tam ; i++)
		str[i] = '\0';
}


void pergunta_tamanho_disco(int * tf_disco)
{
	do
	{
		textcolor(3); printf("\n\tQual tamanho de disco?\n\t--> ");
		textcolor(5); scanf("%d", &*tf_disco);
	
		if(*tf_disco > 1000 || *tf_disco <= 0)
		{
			textcolor(8);
			printf("\n\nTamanho tem que ser entre 1 e 1000");
		}
		
		textcolor(15);
	}while(*tf_disco > 1000 || *tf_disco <= 0);
}

void pega_funcao(char funcao[], char comando[])
{
	int i;
	
	while(comando[i] != '\0' && comando[i] != '\n' && comando[i] != ' ')
	{
		funcao[i] = comando[i];
		i++;
	}
	funcao[i]='\0';
}


void ler_comando(Block disco[], int * topo_blocks_free, int tf_disco,int endereco_inode_dir_raiz, 
				 int * endereco_inode_dir_atual)
{
	char funcao[30], 
		 comando[255];
	
	while(strcmp(comando,"exit") != 0)
	{
		init_str(comando, 30);
		init_str(funcao, 255);
		
		printf("[%s@%s]: ", NOME_USUARIO_PADRAO, NOME_GRUPO_PADRAO);
		fflush(stdin); scanf("%[^\n]s", &comando);
	
		if(strcmp(comando, "") != 0)
		{
			pega_funcao(funcao, comando);
			
			if(strcmp(funcao, "chmod") == 0)
				chmod(disco, endereco_inode_dir_raiz, *endereco_inode_dir_atual, comando);
			
			else if(strcmp(funcao, "vi") == 0)
				visualizar_arquivo_regular(disco, endereco_inode_dir_raiz, *endereco_inode_dir_atual, comando);
			
			else if(strcmp(funcao, "mkdir") == 0)	
				criar_diretorio(disco, &*topo_blocks_free, endereco_inode_dir_raiz, *endereco_inode_dir_atual, comando);
			
			else if(strcmp(funcao, "rmdir") == 0)	
				rmdir(disco, &*topo_blocks_free, endereco_inode_dir_raiz, *endereco_inode_dir_atual, comando);	
			
			else if(strcmp(funcao, "rm") == 0)	
				rm(disco, &*topo_blocks_free, endereco_inode_dir_raiz, *endereco_inode_dir_atual, comando);
			
			else if(strcmp(funcao, "cd") == 0)
				mover_para_diretorio(disco, endereco_inode_dir_raiz, &*endereco_inode_dir_atual, comando);
			
			else if(strcmp(funcao, "bad") == 0)
				bad(disco, tf_disco, endereco_inode_dir_raiz, *endereco_inode_dir_atual, comando);
			
			else if(strcmp(funcao, "touch") == 0)	
				touch(disco, &*topo_blocks_free, endereco_inode_dir_raiz, *endereco_inode_dir_atual, comando);
			
			else if(strcmp(funcao, "ls") == 0)
				ls_and_lsl(disco, *endereco_inode_dir_atual, comando);
			
			else if(strcmp(funcao, "df") == 0)
				df(disco, endereco_inode_dir_raiz, *endereco_inode_dir_atual);
			
			else if(strcmp(funcao, "link") == 0 || strcmp(funcao, "unlink") == 0)
				links(disco, &*topo_blocks_free, endereco_inode_dir_raiz, *endereco_inode_dir_atual, comando);
					
			else if(strcmp(funcao, "clear") == 0)
				system("cls");
				
			else if(strcmp(funcao, "relatorio") == 0)
				mostrar_relatorios(disco, &*topo_blocks_free, endereco_inode_dir_raiz, &*endereco_inode_dir_atual, comando);	
				
			else
				printf("FUNCAO NAO ENCONTRADA.\n");	
		}
	}
}


int main()
{	
	
	int tf_disco,
		topo_blocks_free,
		endereco_inode_dir_atual, //ENDERECO DO INODE DO DIRETORIO ATUAL;
		endereco_inode_dir_raiz; //ENDERECO DO INODE DO DIRETORIO RAIZ;
		
	//DISCO INTEIRO
	Block disco[1000];  // (((============= MUDAR ISSO DEPOIS PARA 1000 ===========) TA DANDO BUG POIS ESTA FERRANDO A PILHA
	
	//TOPO DO ENDERECO DA LISTA DE BLOCOS
	topo_blocks_free = 0;
	
	//TAMANHO FISICO DISCO
//	tf_disco = 1000;
		
	pergunta_tamanho_disco(&tf_disco);	
		
	//INICIAR LISTA DE PILHA DE BLOCOS COM ENDERECOS
	init_blocos_livres(disco, tf_disco);
	
	//CRIAR DIRETORIO RAIZ /
	criar_diretorio_raiz(disco, &topo_blocks_free, &endereco_inode_dir_atual);
	
	//PEGAR ENDERECO DO INODE DO DIRETORIO RAIZ /
	endereco_inode_dir_raiz = endereco_inode_dir_atual;
	
	
	ler_comando(disco, &topo_blocks_free, tf_disco, endereco_inode_dir_raiz, &endereco_inode_dir_atual);
}


