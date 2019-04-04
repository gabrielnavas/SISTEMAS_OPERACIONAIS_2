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
typedef struct inode_extend InodeExtend;

struct inode_principal
{
	char permissoes[11];
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
	
//	char tipo; NAO PRECISA DISSO
	
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
	

	inicio_endereco_block_livres = total_blocos/10;
	fim_blocos = total_blocos/10;
	for(i=0 ; i < fim_blocos && inicio_endereco_block_livres < total_blocos; i++)
	{
		disco[i].no_lista_block.tl=-1;
		disco[i].no_lista_block.prox = i+1;
		
		
		disco[i].no_lista_block.tl++;	
		while(disco[i].no_lista_block.tl < 10 && inicio_endereco_block_livres < total_blocos)
		{
//			strcpy(disco[i].tipo, LISTA_BLOCK_FREE);
			disco[i].no_lista_block.pilha[ disco[i].no_lista_block.tl++ ] = inicio_endereco_block_livres;
			
			inicio_endereco_block_livres++;
		}
		
	}
	
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
	
	qntd_blocks=0;
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
	
	disco[endereco_atual].diretorio.i_numero[0] = endereco_atual;
	disco[endereco_atual].diretorio.i_numero[1] = (endereco_anterior == -1) ? endereco_atual : endereco_anterior ; 	
	strcpy(disco[endereco_atual].diretorio.nome_arq[0], ".");
	strcpy(disco[endereco_atual].diretorio.nome_arq[1], "..");
	disco[endereco_atual].diretorio.tl=2;	
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

void inserir_inode_extend_simples(Block disco[], int * topo_blocks_free, int endereco_inode_atual_simples, int *quantidade_block_indireto)
{
	int i,
		endereco_blocos;
	
	
	for(i=0 ; i < 5 && *quantidade_block_indireto > 0; i++, (*quantidade_block_indireto)--)
	{
		//RETIRA ENDERECO DA LISTA DE BLOCOS;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		
		//ALOCA DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		disco[endereco_inode_atual_simples].inode_extend.b_diretos[i] = endereco_blocos;
		
		//CRIA ARQUIVO REGULAR EM DISCO
		criar_inode_block_arquivo_regular(disco, endereco_blocos);
	}
}

void inserir_inode_extend_duplo(Block disco[], int * topo_blocks_free, int endereco_inode_atual_duplo, int *quantidade_block_indireto)
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
		inserir_inode_extend_simples(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_block_indireto);
	}
}

void inserir_inode_extend_triplo(Block disco[], int * topo_blocks_free, int endereco_inode_atual_triplo, int *quantidade_block_indireto)
{
	int i,
		endereco_blocos;
	
	for(i=0 ; i < 4 && *quantidade_block_indireto > 0 ; i++)
	{
		//RETIRA ENDERECO DA LISTA DE BLOCOS;
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		
		//ALOCA DENTRO DA LISTA DA ESTRUTURA DE INODE EXTENDIDO
		disco[endereco_inode_atual_triplo].inode_extend.b_diretos[i] = endereco_blocos;
		
		//CRIA ARQUIVO REGULAR EM DISCO
		criar_inode_block_arquivo_regular(disco, endereco_blocos);
		
		//INSERIR OS INDIRETOS SIMPLES
		inserir_inode_extend_duplo(disco, &*topo_blocks_free, endereco_blocos, &*quantidade_block_indireto);
	}
	
	if(*quantidade_block_indireto)
	{
		//CHEGOU NO ULTIMO BLOCK, O QUE FAZER AGORA?
	}
}

void criar_inode_arquivo( Block disco[], int * topo_blocks_free, int endereco_inode_atual, int quantidade_blocos)
{	
	//ARQUIVO REGULAR

	int endereco_blocos,
		i;

	disco[endereco_inode_atual].inode.tamanho = quantidade_blocos * 10;
	strcpy(disco[endereco_inode_atual].inode.permissoes, "-rw-r--r--\0");
	strcpy(disco[endereco_inode_atual].inode.data, __DATE__);
	strcpy(disco[endereco_inode_atual].inode.hora, __TIME__);
	strcpy(disco[endereco_inode_atual].inode.nome_usuario, NOME_USUARIO_PADRAO);
	strcpy(disco[endereco_inode_atual].inode.nome_grupo, NOME_GRUPO_PADRAO);
	disco[endereco_inode_atual].inode.contador_link_hard = 1; //vc tinha razao andressa, terrorista.
	
	
	//INSERIR OS BLOCOS DIRETOS QNTD = 5
	for(i=0; i < quantidade_blocos && i < 5 ; i++, quantidade_blocos--)
	{
		//PEGA BLOCK DA LISTA DE BLOCOS
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		
		//ATRIBUI ENDERECO DO BLOCO NA VARIAVEL QUE APONTA
		disco[endereco_inode_atual].inode.b_diretos[i] = endereco_blocos;
		
		//ALOCA O BLOCO REGULAR NO DISCO
		criar_inode_block_arquivo_regular(disco, endereco_blocos);
	}
	
	//BLOCOS INDIRETOS SIMPLES QNTD = 10
	if(quantidade_blocos)
	{
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		disco[endereco_inode_atual].inode.b_indi_simples = endereco_blocos;
		
		inserir_inode_extend_simples(disco, &*topo_blocks_free, endereco_blocos, &quantidade_blocos);	
	}
	

	//BLOCOS INDIRETOS DUPLO QNTD = 125
	if(quantidade_blocos)
	{
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		disco[endereco_inode_atual].inode.b_indi_simples = endereco_blocos;
		
		inserir_inode_extend_duplo(disco, &*topo_blocks_free, endereco_blocos, &quantidade_blocos);	
	}
	
	
	//BLOCO INDIRETO TRIPLO QNTD = 625
	//FALTA TRATAR O ULTIMO ENDERECO CASO O NUMERO DE BLOCOS SE PASSE de 624
	if(quantidade_blocos)
	{
		pop_lista_block(disco, &*topo_blocks_free, &endereco_blocos);
		disco[endereco_inode_atual].inode.b_indi_simples = endereco_blocos;
		
		inserir_inode_extend_triplo(disco, &*topo_blocks_free, endereco_blocos, &quantidade_blocos);
	}
}

void inserir_arquivo_in_estrutura_diretorio(Block disco[], int * topo_blocks_free, 
												int endereco_inode_dir_atual, char nome_arq[255], 
												int quantidade_blocos)
{
	//INSERIR UM ARQUIVO DENTRO DO DIRETORIO ATUAL
	int endereco_arq_novo,
		endereco_dir_atual,
		tl_dir_atual;
	
	//retira endereco para o inode do arquivo novo;
	pop_lista_block(disco, &*topo_blocks_free, &endereco_arq_novo);
	
	//aloca inode do diretorio novo
	criar_inode_arquivo(disco, &*topo_blocks_free, endereco_arq_novo, quantidade_blocos);
	
	//pega endereco real da estrutura, (endereco atual eh do inode)
	endereco_dir_atual = disco[endereco_inode_dir_atual].inode.b_diretos[0];
	
	//insere esse dir novo na estrutura atual;
	strcpy(disco[ endereco_dir_atual ].diretorio.nome_arq[ disco[endereco_dir_atual ].diretorio.tl ], nome_arq);
	disco[ endereco_dir_atual ].diretorio.i_numero[ disco[endereco_dir_atual ].diretorio.tl ] = endereco_arq_novo;
	disco[endereco_dir_atual ].diretorio.tl++;
	
	//tratar quando passa a quantidade de diretorios alocados ============================ (IMPORTANTE)
}

void criar_arquivo( Block disco[], int * topo_blocks_free, int inode_endereco_dir_atual, 
					char nome_arquivo[255], int quantidade_blocos_user)
{	
	if(quantidade_blocks_livres(disco, *topo_blocks_free) >= 1) //minimo para criar um arquivo, 1 bloco para inode
	{		
		
		if(/* TRATAR TAMANHO DE BYTES PARA O ARQUIVO, VERIFICAR SE EXISTE TAMANHO NECESSARIO*/true)
			inserir_arquivo_in_estrutura_diretorio(disco, &*topo_blocks_free, inode_endereco_dir_atual, nome_arquivo, quantidade_blocos_user);
		
	}
	else
		printf("ESPACO EM DISCO INSUFICIENTE.");
}

void inserir_diretorio_in_estrutura_diretorio(Block disco[], int * topo_blocks_free, 
												int endereco_inode_dir_atual, char nome_dir[255])
{
	//INSERIR UM DIRETORIO DENTRO DO DIRETORIO ATUAL.
	
	int endereco_dir_novo,
		endereco_inode_dir_novo,
		tl_dir_atual;
	
	//retira endereco para o inode do diretorio novo;
	pop_lista_block(disco, &*topo_blocks_free, &endereco_dir_novo);
	
	//retira endereco para o diretorio novo
	pop_lista_block(disco, &*topo_blocks_free, &endereco_inode_dir_novo);
	
	//aloca inode do diretorio novo
	criar_inode_diretorio(disco, endereco_inode_dir_novo, endereco_dir_novo);
	
	//aloca estrutura de diretorio do diretorio novo;
	criar_estrutura_diretorio(disco, &*topo_blocks_free, endereco_dir_novo, endereco_inode_dir_atual);
	
	
	//pega endereco real da estrutura, (endereco atual eh do inode)
	endereco_dir_novo = disco[endereco_inode_dir_atual].inode.b_diretos[0];
	
	//insere esse dir novo na estrutura atual;
	strcpy(disco[ endereco_dir_novo ].diretorio.nome_arq[ disco[endereco_dir_novo ].diretorio.tl ], nome_dir);
	disco[ endereco_dir_novo ].diretorio.i_numero[ disco[endereco_dir_novo ].diretorio.tl ] = endereco_inode_dir_novo;
	disco[endereco_dir_novo ].diretorio.tl++;
	
	
	//tratar quando passa a quantidade de diretorios alocados ============================ (IMPORTANTE)
}

void criar_diretorio(Block disco[], int * topo_blocks_free, int inode_endereco_dir_atual, char nome_dir[255])
{
	// minimo para criar um diretorio, 1 block para inode outro para estrutura de dir
	if(quantidade_blocks_livres(disco, *topo_blocks_free) >= 2)
	{		
		inserir_diretorio_in_estrutura_diretorio(disco, &*topo_blocks_free, inode_endereco_dir_atual, nome_dir);
	}
	else
		printf("ESPACO EM DISCO INSUFICIENTE.");
}

//ls
void listar_diretorio(Block disco[], int endereco_dir_atual)
{
	int endereco_struct_dir,
		i,
		j,
		tl_dir;
	
	endereco_struct_dir = disco[endereco_dir_atual].inode.b_diretos[0];
	
	for(i=0 ; i < disco[endereco_struct_dir].diretorio.tl ; i++)
		printf("%s\n", disco[endereco_struct_dir].diretorio.nome_arq[i]);
}

//ls -l
void listar_diretorio_atributos(Block disco[], int endereco_dir_atual)
{
	int endereco_struct_dir,
		endereco_inode,
		i,
		j,
		tl_dir;
	
	endereco_struct_dir = disco[endereco_dir_atual].inode.b_diretos[0];
	
	for(i=0 ; i < disco[endereco_struct_dir].diretorio.tl ; i++)
	{
		if(strcmp(disco[endereco_struct_dir].diretorio.nome_arq[i], ".") != 0 && 
			strcmp(disco[endereco_struct_dir].diretorio.nome_arq[i], "..") !=  0)
		{	
			endereco_inode = disco[endereco_struct_dir].diretorio.i_numero[i];
			
				
			printf("%s ", disco[endereco_inode].inode.permissoes);
			printf("%d ", disco[endereco_inode].inode.contador_link_hard);					 
			printf("%s ", disco[endereco_inode].inode.nome_usuario);
			printf("%s ", disco[endereco_inode].inode.nome_grupo);
			printf("%d ", disco[endereco_inode].inode.tamanho);
			printf("%s ", disco[endereco_inode].inode.data);
			printf("%s ", disco[endereco_inode].inode.hora);
			
			//COR DIFERENTE POR TIPO DE ARQUIVO
			if(disco[endereco_inode].inode.permissoes[0] == '-')
				textcolor(5);
			else if(disco[endereco_inode].inode.permissoes[0] == 'd')
				textcolor(6);
			else
				textcolor(7);
			printf("%s\n" , disco[endereco_struct_dir].diretorio.nome_arq[i]);
			
			textcolor(15);	
													 
		}
		else
			printf("%s\n" , disco[endereco_struct_dir].diretorio.nome_arq[i]);
	}
	
	textcolor(15);	
}

void split_caminho(char caminhos[][255], int * tl_caminhos, char string_caminho[])
{
	int i,
		j,
		tl_aux_caminho,
		tl_string_caminho;
	
	char aux_caminho[255];
	
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

// cd
void mover_para_diretorio(Block disco[], int endereco_inode_raiz ,int * endereco_inode_dir_atual,char string_caminho[])
{
	char caminhos[100][255],
		 dir_valido;
	
	int i_node_atual,
		i_node_atual_aux,
		dir_atual,
		i,
		j,
		tl_caminhos;
	
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
	
	dir_valido=1;
	while(i < tl_caminhos && dir_valido)
	{
//			printf("%s\n", caminhos[i]);
		if(strcmp(caminhos[i], "/") != 0)
		{
			dir_atual = disco[i_node_atual].inode.b_diretos[0];
			
			//procurar dentro do diretorio o nome do diretorio i
			for(j=0 ; j < disco[dir_atual].diretorio.tl && 
				strcmp(disco[dir_atual].diretorio.nome_arq[j], caminhos[i]) != 0; j++);
				
			if(j < disco[dir_atual].diretorio.tl && strcmp(disco[dir_atual].diretorio.nome_arq[j], caminhos[i]) == 0)
			{
				//verifica se realmente o nome achado eh um diretorio, olhando no inode do diretorio
				i_node_atual_aux = disco[dir_atual].diretorio.i_numero[j];
				if(disco[i_node_atual_aux].inode.permissoes[0] == 'd')
					i_node_atual = i_node_atual_aux;
				else
					dir_valido=0;
			}
				
			else
				dir_valido=0;
		}
		
		i++;
	}	

	
	if(dir_valido)
		*endereco_inode_dir_atual = i_node_atual;
}

void mudar_permissao(Block disco[], int endereco_inode_raiz ,int endereco_inode_dir_atual, 
					 char str_arq[], char str_permissao[])
{
	int i_node_atual,
		i,
		j,
		pos_permissoes_inode;
	
	char retira_insere,
		 quem[3], //0 = user, 1 = group, 2 = other;
		 permissoes[3];
		 
	retira_insere='-';
	for(i=0 ; i < 3 ; i++)
		quem[i] = permissoes[i] = '-';
	
	i_node_atual = endereco_inode_dir_atual;
	mover_para_diretorio(disco, endereco_inode_raiz, &i_node_atual, str_arq);
	
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
						disco[i_node_atual].inode.permissoes[pos_permissoes_inode+j] = permissoes[j];	
				
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

void chmod(Block disco[], int endereco_inode_dir_raiz ,int endereco_inode_dir_atual, char comando[])
{
	//FALTA DIFERENCIAR SE EH DIRETORIO OU ARQUIVO.
	char arquivo[255],
		 argumento_comando[255];
		 caminho[255];
		 
	split_comando_chmod(comando, arquivo, argumento_comando);
	split_caminho(comando, )
	
	mover_para_diretorio(disco, endereco_inode_dir_raiz, &endereco_inode_dir_atual, "/bin\0");
	
//	mudar_permissao(disco, endereco_inode_dir_raiz, endereco_inode_dir_atual, "arquivo_bin_dir_bin.bin\0", "+ug X");
	mudar_permissao(disco, endereco_inode_dir_raiz, endereco_inode_dir_atual, caminho_arquivo, argumento_comando);
	
}

void testar_lista_blocos(Block disco[], int topo_blocks_free)
{
	int endereco;
	int i;
	int endereco_inode_dir_atual;
	int dir_padrao;
	
	criar_diretorio_raiz(disco, &topo_blocks_free, &dir_padrao);
	listar_diretorio_atributos(disco, dir_padrao);
	
	
	
	exibir_blocos_livres(disco, topo_blocks_free);//
	
	printf("\n\nQUANTIDADE BLOCOS LIVRES: %d\n", quantidade_blocks_livres(disco, topo_blocks_free));
	
	for(i=0 ; i < 9 ; i++)
	{
		pop_lista_block(disco, &topo_blocks_free, &endereco);
		printf("%d\n", endereco);
	}
	
	
	printf("\n\nQUANTIDADE BLOCOS LIVRES: %d\n", quantidade_blocks_livres(disco, topo_blocks_free));
	
	pop_lista_block(disco, &topo_blocks_free, &endereco);
	printf("%d\n", endereco);
			
	printf("\n\nQUANTIDADE BLOCOS LIVRES: %d\n", quantidade_blocks_livres(disco, topo_blocks_free));
}

int main()
{	
	
	int tf_disco,
		topo_blocks_free,
		endereco_inode_dir_atual, //ENDERECO DO INODE DO DIRETORIO ATUAL;
		endereco_inode_dir_raiz; //ENDERECO DO INODE DO DIRETORIO RAIZ;
		
	//DISCO INTEIRO
	Block disco[500];
	
	//TOPO DO ENDERECO DA LISTA DE BLOCOS
	topo_blocks_free = 0;
	
	//TAMANHO FISICO DISCO
	tf_disco = 500;
		
	//INICIAR LISTA DE PILHA DE BLOCOS COM ENDERECOS
	init_blocos_livres(disco, tf_disco);
	
	//CRIAR DIRETORIO RAIZ /
	criar_diretorio_raiz(disco, &topo_blocks_free, &endereco_inode_dir_atual);
	
	//PEGAR ENDERECO DO INODE DO DIRETORIO RAIZ /
	endereco_inode_dir_raiz = endereco_inode_dir_atual;
	
	
	
	//TESTES
	criar_arquivo(disco, &topo_blocks_free, endereco_inode_dir_atual, "arquivo_bin.bin\0", 20);
	criar_diretorio(disco, &topo_blocks_free, endereco_inode_dir_atual, "bin\0");
	
//	listar_diretorio_atributos(disco, endereco_inode_dir_atual);
	
	mover_para_diretorio(disco, endereco_inode_dir_raiz, &endereco_inode_dir_atual, "bin\0");
	
//	listar_diretorio_atributos(disco, endereco_inode_dir_atual);
	
	//printf("%d", disco[disco[endereco_inode_dir_atual].inode.b_diretos[0]].diretorio.tl);
	
	criar_arquivo(disco, &topo_blocks_free, endereco_inode_dir_atual, "arquivo_bin_dir_bin.bin\0", 15);
	criar_diretorio(disco, &topo_blocks_free, endereco_inode_dir_atual, "abc\0");
	
	mover_para_diretorio(disco, endereco_inode_dir_raiz, &endereco_inode_dir_atual, "abc\0");
	
//	listar_diretorio_atributos(disco, endereco_inode_dir_atual);
	
	criar_arquivo(disco, &topo_blocks_free, endereco_inode_dir_atual, "arquivo_abc.bin\0", 20);
	criar_diretorio(disco, &topo_blocks_free, endereco_inode_dir_atual, "cba\0");
	
	
	mover_para_diretorio(disco, endereco_inode_dir_raiz, &endereco_inode_dir_atual, "/bin\0");
	
	chmod(disco, endereco_inode_dir_raiz, endereco_inode_dir_atual, "chmod +ug X arquivo_bin_dir_bin.bin\0");
	
	listar_diretorio_atributos(disco, endereco_inode_dir_atual);
	
	//TESTAR GERENCIAMENTO DE ESPACO LIVRE
//	testar_lista_blocos(disco, topo_blocks_free);
	
//	ler_comando();
}




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


