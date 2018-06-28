
#include "fs.h"
#include "disk.h"
//
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>

/* editado */
#include <iostream>
#include <vector>
using namespace std;


#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024


struct fs_superblock {
	int magic;
	int nblocks;
	int ninodeblocks;
	int ninodes;
};

struct fs_inode {
	bool isvalid;
	int size;
	int direct[POINTERS_PER_INODE];
	int indirect;
};

union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	int pointers[POINTERS_PER_BLOCK];
	char data[DISK_BLOCK_SIZE];
};

//variável que assinala se o sistema está ou não mantado:
volatile bool _mounted = false;

/*
 *
 * Convenção:
 *	false: livre
 *	true: ocupado
 *
 */

//mapa de bits do disco:
vector<bool> bitmap;

/*
 * 
 * Mapa dos inodos:
 * guarda a posicao da memoria pra onde esse inodo aponta.
 * 	Se tiver 0: inodo livre
 * 	Se for diferente: inodo valido e contem bloco onde esta inserido
 *
 */
vector<int> inodemap;
/* carrega o seguinte inode da memoria */
void inode_load( int inumber, struct fs_inode *inode_ler ) {
	/* inode alocado */
	inode_ler = new struct fs_inode;
	/* bloco de leitura */
	union fs_block leitura;

	/* A FAZER ABAIXO: Verificar se inumber varia de 0-127 ou de 1-128. Se for a primeira tem que mudar o numero de bloco e posicao */

	/* \/\/ */

	/* Acha o bloco em que o inode se encontra. Soma 1 pra tirar o superbloco */
	int numbloco = inumber/INODES_PER_BLOCK + 1;
	/* Acha a posicao no bloco */
	int posicao = inumber%INODES_PER_BLOCK;

	/* /\/\ */

	/* efetua a leitura do bloco */
	disk_read(numbloco,leitura.data);
	/* aponta inode_ler para a posicao no bloco  */
	inode_ler = leitura.inode+posicao;

}
void inode_save( int inumber, struct fs_inode inode_esc ) {
	/* bloco de escrita */
	union fs_block escrita;

	/* A FAZER ABAIXO: Verificar se inumber varia de 0-127 ou de 1-128. Se for a primeira tem que mudar o numero de bloco e posicao */

	/* \/\/ */

	/* Acha o bloco em que o inode se encontra. Soma 1 pra tirar o superbloco */
	int numbloco = inumber/INODES_PER_BLOCK + 1;
	/* Acha a posicao no bloco */
	int posicao = inumber%INODES_PER_BLOCK;

	/* /\/\ */

	/* efetua a leitura do bloco */
	disk_read(numbloco,escrita.data);

	/* atribui o valor desse inode no bloco */
	escrita.inode[posicao] = inode_esc;

	/* escreve o dado na memoria */
	disk_write(numbloco,escrita.data);

	/* libera o inode */
	//delete &inode_esc;
}

/*
 * Cria um novo sistema de arquivos no disco, destruindo qualquer dado que estiver presente. Reserva
 * dez por cento dos blocos para inodos, libera a tabela de inodos, e escreve o superbloco. Retorna um para
 * sucesso, zero caso contrario. Note que formatar o sistema de arquivo n~ao faz com que ele seja montado.
 * Tambem, uma tentativa de formatar um disco que ja foi montado n~ao deve fazer nada e retornar falha.
 *
 */
int fs_format()
{
	int tamanho = disk_size();
	/* tamanho para inodos */
	//int tamanho_inodos = tamanho/10;
	union fs_block bloco0;
	disk_read(0,bloco0.data);
	//int magico = bloco0.super.magic;
	/* Fazer todos os i-nodes serem invalidos */

	return 0;
}

/*
 * Varre um sistema de arquivo montado e reporta como os inodos e os blocos est~ao organizados. Se
 * voc^e conseguir escrever esta func~ao, voc^e ja ganhou metade da batalha! Assim que voc^e conseguir varrer
 * e reportar as estruturas do sistema de arquivos, o resto e facil. Sua saida do fs debug deve ser similar ao
 * seguinte:
 *
 */
void fs_debug()
{
	/* Bloco inicial */
	union fs_block block;
	/* vetor com os outros blocos na memoria */
	union fs_block *block_vec;
	/* le a parte zero que e o superbloco */
	disk_read(0,block.data);

	cout<<endl<<"\t--->\tDebug:"<<endl;
	if (_mounted)
	{
		cout<<"FS mounted"<<endl;
	}else cout<<endl<<"FS not mounted"<<endl;

	cout<<"superblock:"<<endl;
	cout<<"    "<<block.super.nblocks<<" blocks"<<endl;
	cout<<"    "<<block.super.ninodeblocks<<" inode blocks"<<endl;;
	cout<<"    "<<block.super.ninodes<<" inodes"<<endl;;


	/* Aloca os espacos dos blocos para o vetor de blocos de inodes */
	block_vec = new union fs_block[block.super.ninodeblocks];
	int i,j,k;
	int num_inodos_percorridos = 0;
	/* percorre todos os blocos que contem inodes. Comeca do 1 pq o 0 e o superbloco */
	for (i = 0; i < block.super.ninodeblocks; i++) {
		/* Coloca o valor do disco no vetor de blocos */
		/* le a porcao i+1 pq ja foi lida a porcao 0 */
		disk_read(i+1,block_vec[i].data);

		/* percorre todos os inodos do bloco */
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			/* Verifica se o inodo e valido */
			if (block_vec[i].inode[j].isvalid) {
				cout << "inode " << num_inodos_percorridos << ": " << endl;
				/* Escreve o tamanho */
				cout << "\tsize: " << block_vec[i].inode[j].size << " bytes" << endl;

				/* Escreve quais sao os blocos diretos */
				cout << "\tdirect blocks:"<<endl;
				for (k = 0; k < POINTERS_PER_INODE; k++) {
					/* Se o numero for zero não exibe. */
					if (block_vec[i].inode[j].direct[k] != 0) cout << "\t\t" << block_vec[i].inode[j].direct[k]<<endl;
				}
				cout << endl;

				/* Aqui comeca a parte que pega um pouco. Tenho que achar os blocos indiretos. */
				/* Se indirect for 0 entao nao tem. Mas se nao for isso sera o numero do bloco */
				/* com ponteiros para os outros blocos */

				/* Verifica se existem blocos indiretos */
				if (block_vec[i].inode[j].indirect != 0) {
					/* bloco de indirecao */
					union fs_block bloco_indirecao;
					/* Efetua a leitura */
					disk_read(block_vec[i].inode[j].indirect,bloco_indirecao.data);

					/* Numero de ponteiros validos */
					int num_ponteiros = 0;
					/* Agora trata bloco_indirecao como campo de ponteiros e conta os ponteiros diferentes de zero */
					for (k = 0; k < POINTERS_PER_BLOCK; k++) {
						/* Sera ponteiro valido se for maior que zero */
						if (bloco_indirecao.pointers[k] > 0) num_ponteiros++;
					}

					/* Mostra qual e o bloco de indirecao */
					cout << "\tindirect block: " << block_vec[i].inode[j].indirect << endl;

					/* So exibe o proximo se bloco de indirecao tiver um ponteiro valido */
					if (num_ponteiros > 0) {
						cout << "\tindirect data blocks:"<<endl;
						/* Exibe agora os numeros diferentes de zero */
						for (k = 0; k < POINTERS_PER_BLOCK; k++) {
							/* Sera ponteiro valido se for maior que zero */
							if (bloco_indirecao.pointers[k] > 0) cout << "\t\t" << bloco_indirecao.pointers[k]<<endl;
						}
						cout << endl;
					}
				}

			}
			num_inodos_percorridos++;
			if (num_inodos_percorridos == block.super.ninodes) break;
		}
		if (num_inodos_percorridos == block.super.ninodes) break;
	}

	delete block_vec;
}

/*
 * Examina o disco para um sistema de arquivo. Se um esta presente, l^e o superbloco, constroi um
 * bitmap de blocos livres, e prepara o sistema de arquivo para uso. Retorna um em caso de sucesso, zero
 * caso contrario. Note que uma montagem bem-sucedida e um pre-requisito para as outras chamadas.
 *
 */
int fs_mount()
{

	int i,j,k;		  //contadores
	fs_block s_block; //superbloco
	fs_block i_block; //bloco de inode
	fs_block id_block;//bloco indireto

	//Lê o primeiro bloco (super bloco):
 	disk_read(0, s_block.data);
	//Verifica se já está montado ou se o disco não tem a identificação esperada:
	if (_mounted || s_block.super.magic != FS_MAGIC )
	{
		//retorna insucesso:
		return 0;
	}
	//altera o tamanho do bitmap para o número de blocos do disco:
	bitmap.resize(s_block.super.nblocks,false);
	//percorre o superbloco e os blocos de inode:
	for(i=0; i<(s_block.super.ninodeblocks+1); i++)
	{
		//salva eles no mapa de blocos:
		bitmap[i]=true;
	}
	//percorrer os blocos de inodo:
	for(i=1; i<(s_block.super.ninodeblocks+1); i++)
	{
		disk_read(i, i_block.data);//le um bloco de inodo
		//percorre o inodo atual:
		for(int j = 0; j < INODES_PER_BLOCK; j++)
		{
			//adiciona uma posição no mapa de inodos;
			//inodemap.push_back(0);
			//se o inodo atual for válido:
			if(i_block.inode[j].isvalid)
			{	
				//salva o bloco do inodo
				//inodemap[i-1+j]=i;                 			<------ O Leal tinha me dito que criou esse mapa de inodos, não sei se é necessário, confira isso por favor!
				//percorre os blocos diretos:
				for(int k=0; k<POINTERS_PER_INODE; k++)
				{
					//se ele apontar para algum bloco válido:
					if(i_block.inode[j].direct[k]>0 && i_block.inode[j].direct[k]<=s_block.super.nblocks) 
					{
						//salva a posição no mapa de blocos:
						bitmap[i_block.inode[j].direct[k]] = true;
					}
				}
				//verifica se existem blocos indiretos:
				if(i_block.inode[j].indirect>s_block.super.ninodeblocks && i_block.inode[j].indirect<s_block.super.nblocks)
				{
					//lê o bloco indireto:
					disk_read(i_block.inode[j].indirect, id_block.data);
					for(int k=0; k<POINTERS_PER_BLOCK; k++)
					{
						//se o valor do bloco indireto for válido:
						if(id_block.pointers[k]>s_block.super.ninodeblocks && id_block.pointers[k]<s_block.super.nblocks)
						{
							//salva a posição no mapa de blocos:
							bitmap[id_block.pointers[k]] = true;
						}
					}

				}

			}
		}
	}
	//coloca o marcador de montado como verdadeiro:
	_mounted=true;
	//retorna sucesso:
	return 1;
}

int fs_create()
{

	//Verifica se está montado:
	if (!_mounted)
	{
		//retorna insucesso:
		return 0;
	}
	int i,j,k;		  	//contadores
	int inode_n=0;  	//contador de inodos percorridos 
	fs_block s_block; 	//superbloco
	fs_block i_block; 	//bloco de inodo
	//Lê o primeiro bloco (super bloco):
	disk_read(0, s_block.data);
	//Lê o primeiro bloco de inodos:
	disk_read(1, i_block.data);
	//Torna o inodo zero "ocupado" para que ele não seja criado devido ao problema com o código de erro:
	i_block.inode[0].isvalid=true;
	//Salva o bloco com o inodo zero "ocupado":
	disk_write(1,i_block.data);
	//percorrer os blocos de inodo:
	for(i=1; i<(s_block.super.ninodeblocks+1); i++)
	{
		//le um bloco de inodo:
		disk_read(i, i_block.data);
		//percorre o inodo atual:
		for(int j = 0; j < INODES_PER_BLOCK; j++)
		{
			//se o inodo atual não for válido:
			if(!i_block.inode[j].isvalid)
			{
				//torna ele válido:
				i_block.inode[j].isvalid=true;
				i_block.inode[j].size=0;
				//salva o bloco do inodo
				//inodemap[i-1+j]=i;    <------ Conferir se precisa desse mapa de inodos!
				//percorre os blocos diretos:
				for(int k=0; k<POINTERS_PER_INODE; k++)
				{
					//faz ele apontar para bloco inválido (esvazia):
					i_block.inode[j].direct[k]=0;
				}
				//faz o bloco indireto para bloco inválido (esvazia):
				i_block.inode[j].indirect=0;
				//salva o inodo em disco:
				disk_write(i,i_block.data);
				//retorna sucesso, o número do inodo, pois encontrou um inodo vazio e o criou
				return inode_n;
			}
			//incrementa o numero de inodos percorridos
			inode_n++;
		}
	}
	//retorna insucesso pois percorreu todos os inodos de todos os blocos e não encontrou um inodo vazio e o criou
	return 0;
}

int fs_delete( int inumber )
{
	return 0;
}

int fs_getsize( int inumber )
{
	return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	return 0;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	return 0;
}
