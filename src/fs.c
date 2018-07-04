

#include "fs.h"
#include "disk.h"
//
#include <algorithm>
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

struct im_elem {
	int bloco_im;
	bool im_valid;
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
vector<im_elem> inodemap;


/* quant de bytes livres */
int espacoLivre() {
	int i;
	int espaco = 0;
	for (i = 1; i < bitmap.size(); i++) {
		if (bitmap[i] == false) {
			espaco++; 
		}
	}
	return espaco*DISK_BLOCK_SIZE;
}

/* proximo blocodisponivel */
int blocoDisponivel() {
	int i;
	for (i = 1; i < bitmap.size(); i++) {
		if (bitmap[i] == false) return i;
	}
	return 0;
}

void printinodemap() {
	int i;
	cout << "\n inodemap: ";
	cout << "X" << " ";
	for (i = 1; i < inodemap.size(); i++) {
		cout << ((inodemap[i].im_valid == true)?1:0) << " ";
	}
	cout << endl << endl;
}

void printbitmap() {
	int i;
	cout << "\n block bitmap: ";
	for (i = 0; i < bitmap.size(); i++) {
		cout << ((bitmap[i] == true)?1:0) << " ";
	}
	cout << endl << endl;
}

int buscatamanho( int inumber ) {
	/* blocos utilizados */
	union fs_block inode_block;
	union fs_block direto;
	int indice;
	int i,j;
	/* leitura do bloco de inodes */
	disk_read(inodemap[inumber].bloco_im,inode_block.data);
	indice = inumber - INODES_PER_BLOCK*inodemap[inumber].bloco_im;
	/* Faz a leitura dos blocos diretos do arquivo */
	for (i = 0; i < POINTERS_PER_INODE; i++) {
		
	}
	return 0;
}

/* carrega o seguinte inode da memoria */
void inode_load( int inumber, struct fs_inode *inode_ler ) {
	/* nao aloca memoria */
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
	/* o conteudo de inode_ler recebe o inode do bloco  */
	*inode_ler = leitura.inode[posicao];
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
int fs_format() {
	/* Verifica se está montado: */
	if (_mounted)
	{
		/* retorna insucesso: */
		return 0;
	}
	
	int i,j;
	
	/* numero de blocos do disco */
	int tamanho = disk_size();
	/* 
	 * tamanho para inodos
	 * Se tiver dez ou menos blocos entao reserva um para inodes
	 */
	int tamanho_inodos = (tamanho>=10)?tamanho/10:1;
	
	
	
	/* novo bloco inicial */
	union fs_block novobloco0;
	
	/* setando informacoes do novo disco */
	novobloco0.super.magic = FS_MAGIC;
	novobloco0.super.nblocks = tamanho;
	novobloco0.super.ninodeblocks = tamanho_inodos;
	novobloco0.super.ninodes = 0;
	
	/* Fazer todos os i-nodes do disco serem invalidos */
	union fs_block antigobloco0;
	union fs_block antigoinodeblock;
	union fs_block bloco_inodes;
	
	/* faz leitura do antigo bloco 0 */
	disk_read(0,antigobloco0.data);
	
	/* percorre todos os blocos de inodes */
	for (i = 1; i < (antigobloco0.super.ninodeblocks+1); i++) {
		/* faz leitura do bloco de inodes */
		disk_read(i,bloco_inodes.data);
		
		/* percorre todos os inodos no bloco */
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			/* faz o inode ser invalido */
			bloco_inodes.inode[j].isvalid = 0;
		}
		
		/* escreve o novo bloco de inodes */
		disk_write(i,bloco_inodes.data);
	}
	
	/* escreve o superbloco */
	disk_write(0,novobloco0.data);
	

	return 1;
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
	union fs_block block_vec;
	/* le a parte zero que e o superbloco */
	disk_read(0,block.data);

	if (_mounted)
	{
		cout<<"FS mounted"<<endl;
	}else cout<<endl<<"FS not mounted"<<endl;
	
	printinodemap();
	printbitmap();
	
	cout<<endl<<"-->Debug:"<<endl;

	cout<<"superblock:"<<endl;
	cout<<"    "<<block.super.nblocks<<" blocks"<<endl;
	cout<<"    "<<block.super.ninodeblocks<<" inode blocks"<<endl;;
	cout<<"    "<<block.super.ninodes<<" inodes"<<endl;;


	int i,j,k;
	int num_inodos_percorridos = 0;
	/* percorre todos os blocos que contem inodes. Comeca do 1 pq o 0 e o superbloco */
	for (i = 0; i < block.super.ninodeblocks; i++) {
		/* Coloca o valor do disco no vetor de blocos */
		/* le a porcao i+1 pq ja foi lida a porcao 0 */
		disk_read(i+1,block_vec.data);

		/* percorre todos os inodos do bloco */
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			/* Verifica se o inodo e valido */
			if (block_vec.inode[j].isvalid) {
				cout << "inode " << num_inodos_percorridos << ": " << endl;
				/* Escreve o tamanho */
				cout << "\tsize: " << block_vec.inode[j].size << " bytes" << endl;

				/* Escreve quais sao os blocos diretos */
				cout << "\tdirect blocks:"<<endl;
				for (k = 0; k < POINTERS_PER_INODE; k++) {
					/* Se o numero for zero não exibe. */
					if (block_vec.inode[j].direct[k] != 0) cout << "\t\t" << block_vec.inode[j].direct[k]<<endl;
				}
				cout << endl;

				/* Aqui comeca a parte que pega um pouco. Tenho que achar os blocos indiretos. */
				/* Se indirect for 0 entao nao tem. Mas se nao for isso sera o numero do bloco */
				/* com ponteiros para os outros blocos */

				/* Verifica se existem blocos indiretos */
				if (block_vec.inode[j].indirect != 0) {
					/* bloco de indirecao */
					union fs_block bloco_indirecao;
					/* Efetua a leitura */
					disk_read(block_vec.inode[j].indirect,bloco_indirecao.data);

					/* Numero de ponteiros validos */
					int num_ponteiros = 0;
					/* Agora trata bloco_indirecao como campo de ponteiros e conta os ponteiros diferentes de zero */
					for (k = 0; k < POINTERS_PER_BLOCK; k++) {
						/* Sera ponteiro valido se for maior que zero */
						if (bloco_indirecao.pointers[k] > 0) num_ponteiros++;
					}

					/* Mostra qual e o bloco de indirecao */
					cout << "\tindirect block: " << block_vec.inode[j].indirect << endl;

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
	
	/* Adiciona o inode zero ocupado no bitmap */
	struct im_elem elem0;
	elem0.bloco_im = 0;
	elem0.im_valid = true;
	inodemap.push_back(elem0);
	
	//percorrer os blocos de inodo:
	for(i=1; i<(s_block.super.ninodeblocks+1); i++)
	{
		disk_read(i, i_block.data);//le um bloco de inodo
		//percorre o inodo atual:
		for(int j = 0; j < INODES_PER_BLOCK; j++)
		{
			struct im_elem novo_elem;
			novo_elem.im_valid = false;
			novo_elem.bloco_im = 0;
			//adiciona uma posição no mapa de inodos;
			inodemap.push_back(novo_elem);
			//se o inodo atual for válido:
			if(i_block.inode[j].isvalid)
			{	
				/* salva o bloco do inodo */
				inodemap[(i-1)*POINTERS_PER_INODE+j].bloco_im=i;
				inodemap[(i-1)*POINTERS_PER_INODE+j].im_valid=true;               
				/* Deixei e arrumei! ^^ */
				
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
					/* Adiciona esse bloco indireto no bitmap */
					bitmap[i_block.inode[j].indirect] = true;
					
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

/*
 * Cria um novo inodo de comprimento zero. Em caso de sucesso, retorna o inumero (positivo). Em
 * caso de falha, retorna zero. (Note que isto implica que zero não pode ser um inumero valido.)
 * 
 */
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
		for( j = 0; j < INODES_PER_BLOCK; j++)
		{
			//se o inodo atual não for válido:
			if(!i_block.inode[j].isvalid)
			{
				//torna ele válido:
				i_block.inode[j].isvalid=true;
				//altera seu tamanho para 0:
				i_block.inode[j].size=0;
				//salva o bloco do inodo no mapa:
				inodemap[(i-1)*POINTERS_PER_INODE+j].bloco_im=i;
				//coloca o inodo como ocupado no mapa:
				inodemap[(i-1)*POINTERS_PER_INODE+j].im_valid=true;   
				//percorre os blocos diretos:
				for( k=0; k<POINTERS_PER_INODE; k++)
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

/*
 * Deleta o inodo indicado pelo inumero. Libera todo o dado e blocos indiretos atribuidos a este
 * inodo e os retorna ao mapa de blocos livres. Em caso de sucesso, retorna um. Em caso de falha, returna
 * 0.
 * 
 */
int fs_delete( int inumber )
{
	//Verifica se está montado, se a posição é válida ou se o inodo é válido:
	if (!_mounted||inumber>=inodemap.size()||!inodemap[inumber].im_valid||inumber==0)
	{
		//retorna insucesso:
		return 0;
	}
	int i;		  		//contadores
	fs_block i_block; 	//bloco de inodo
	fs_block id_block;	//bloco indireto
	//posição do inodo no bloco:
	int inodepos = inumber%INODES_PER_BLOCK;
	//Lê o bloco de inodos:
	disk_read(inodemap[inumber].bloco_im, i_block.data);
	//lê o bloco indireto:
	disk_read(i_block.inode[inumber].indirect, id_block.data);
	//percorre os blocos diretos:
	for(i=0; i<POINTERS_PER_INODE; i++)
	{
		//faz ele apontar para bloco inválido (esvazia):
		i_block.inode[inodepos].direct[i]=0;
	}
	//percorre o bloco indireto:
	for(i=0; i<POINTERS_PER_BLOCK; i++)
	{
		//verifica se é válido:
		if(id_block.pointers[i]>0){
			//apaga o bloco do bitmap:
			bitmap[id_block.pointers[i]] = false;
		}
	}
	//faz o bloco indireto para bloco inválido (esvazia):
	i_block.inode[inodepos].indirect=0;
	//torna ele inválido:
	i_block.inode[inodepos].isvalid=false;
	//altera seu tamanho para 0:
	i_block.inode[inodepos].size=0;
	//torna ele invalido no mapa de inodo:
	inodemap[inumber].im_valid=false;   
	//grava o bloco de inodo:
	disk_write(inodemap[inumber].bloco_im,i_block.data);
	//retorna sucesso:
	return 1;
}

/*
 * Retorna o tamanho logico do inodo especificado, em bytes. Note que zero e um tamanho logico
 * valido para um inodo! Em caso de falha, retorna -1.
 * 
 */
int fs_getsize( int inumber )
{
	//Verifica se está montado:
	if (!_mounted)
	{
		//retorna insucesso:
		return -1;
	}
	/* blocos utilizados */
	union fs_block bloco0;
	union fs_block inode_block;
	/* leitura do primeiro bloco */
	disk_read(0,bloco0.data);
	/* faz a leitura do bloco no bitmap */
	disk_read(inodemap[inumber].bloco_im,inode_block.data);
	int indice = inumber - INODES_PER_BLOCK*inodemap[inumber].bloco_im;
	/* retorna o parametro size do indice */
	return inode_block.inode[indice].size;
}

/*
 * L^e dado de um inodo valido. Copia \length" bytes do inodo para dentro do ponteiro \data",
 * comecando em \oset" no inodo. Retorna o numero total de bytes lidos. O Numero de bytes efetivamente
 * lidos pode ser menos que o numero de bytes requisitados, caso o fim do inodo seja alcancado. Se o inumero
 * dado for invalido, ou algum outro erro for encontrado, retorna 0.
 * 
 */
int fs_read( int inumber, char *data, int length, int offset )
{
	/* Verifica: se esta montado,se data e valido, se o comprimento e valido, inodo valido e tambem o offset */
	if (!_mounted || data == NULL || length <= 0 || offset < 0 || inumber <= 0) return 0;
	struct fs_inode analisado;
	union fs_block direct;
	union fs_block indirect_base;
	union fs_block indirect_block;
	/* quantos bytes faltam */
	int remanescente = length;
	/* posicao em que estou escrevendo */
	int pos = offset;
	/* Acha os ponteiros diretos */
	int i;
	
	/* Recebe o inodo analisado */
	inode_load(inumber,&analisado);
	//cout << "." << endl;
	/* Se a leitura for maior que o tamanho de blocos do inodo retorna falha: */
	if (length > analisado.size) {
		length = analisado.size;
		remanescente = length;
		length -= offset;
	}
	else if (length > (analisado.size - offset)) {
		length = analisado.size - offset;
	}

	//cout << length;
	/* Mesma coisa pro offset */
	//cout << "Offset vale " << offset << " e o tamanho do arquivo analisado e " << analisado.size << endl;
	if (offset >= analisado.size) return 0;

	/* valor inicial de i */
	int i0 = offset/DISK_BLOCK_SIZE;
	/* posicao inicial que procura */
	pos = offset%DISK_BLOCK_SIZE;

	/* se o bloco for menor que o numero de ponteiros */
	if (i0 < POINTERS_PER_INODE) {
		/* Percorre os blocos diretos do inodo */
		for (i = i0; i < POINTERS_PER_INODE; i++) {
			/* Se nao tem mais remanescente acaba a funcao com sucesso */
			if (remanescente == 0) return (length - remanescente);  

			/* Verifica se o bloco e valido */
			if (analisado.direct[i] > 0) {
				/* Faz a leitura do bloco */
				disk_read(analisado.direct[i],direct.data);
				/* Se o tamanho e menor que o tamanho do bloco */
				if (remanescente < DISK_BLOCK_SIZE) {
					/* Pega "remanescente" bytes do direct */
					//cout << "\nCopiando " << remanescente << " bytes do bloco direto " << analisado.direct[i] << endl;
					copy(direct.data,direct.data+remanescente,data+pos);
					return (length);
				}
				else {
					//cout << "\nCopiando " << DISK_BLOCK_SIZE << " bytes do bloco direto " << analisado.direct[i] << endl;
				}
				/* Faz a copia do bloco inteiro para o ponteiro */
				copy(direct.data,direct.data+DISK_BLOCK_SIZE,data+pos);
				pos += DISK_BLOCK_SIZE;
				remanescente -= DISK_BLOCK_SIZE;
			}
		}
	}
	/* desconta os blocos de inodo de i0 */
	i0 -= POINTERS_PER_INODE;
	//cout << "Faltam " << remanescente << "bytes para ler.\n";
	/* Se nao tem blocos indiretos finaliza */
	if (analisado.indirect <= 0) return (length-remanescente);
	/* Se chegou aqui ja percorreu todos os blocos diretos. Pega os indiretos */
	/* Faz a leitura do bloco de indirecao */
	disk_read(analisado.indirect,indirect_base.data);
	/* Percorre todos os blocos indiretos */
	for (i = 0; i < POINTERS_PER_BLOCK; i++) {
		/* Se nao tem mais remanescente acaba a funcao com sucesso */
		if (remanescente == 0) return length; 
		/* Verifica se e um bloco valido */
		if (indirect_base.pointers[i] > 0) {
			/* faz a leitura do bloco */
			disk_read(indirect_base.pointers[i],indirect_block.data);
			/* Se o tamanho que falta e menor que o tamanho do bloco */
			if (remanescente < DISK_BLOCK_SIZE) {
				/* Pega "remanescente" bytes do direct */
				//cout << "\nCopiando " << remanescente << " bytes do bloco indireto " << indirect_base.pointers[i] << endl;
				copy(indirect_block.data,indirect_block.data+remanescente,data+pos);
				return (length);
			}
			else {
				//cout << "\nCopiando " << DISK_BLOCK_SIZE << " bytes do bloco indireto " << indirect_base.pointers[i] << endl;
			}
			/* Faz a copia do bloco inteiro para o ponteiro */
			copy(indirect_block.data,indirect_block.data+DISK_BLOCK_SIZE,data+pos);
			pos += DISK_BLOCK_SIZE;
			remanescente -= DISK_BLOCK_SIZE;
		}
	}
	//cout << "retornou " << length  << endl;
	return (length);
}

/*
 * Escreve dado para um inodo valido. Copia \length" bytes do ponteiro \data" para o inodo
 * comecando em \offset" bytes. Aloca quaisquer blocos diretos e indiretos no processo. Retorna o numero
 * de bytes efetivamente escritos. O numero de bytes efetivamente escritos pode ser menor que o numero de
 * bytes requisitados, caso o disco se torne cheio. Se o inumero dado for invalido, ou qualquer outro erro for
 * encontrado, retorna 0.
 * 
 */
int fs_write( int inumber, const char *data, int length, int offset )
{
	if (inumber <= 0 || !_mounted || data == NULL || length <= 0 || offset < 0)
	{
		return 0;
	}

	cout<<"length: "<<length<<endl;
	cout<<"livre: "<<espacoLivre()<<endl;

	fs_inode inode_block;		//bloco de inodo
	fs_block indirect_block;	//bloco indiretos
	fs_block data_block;		//bloco de dados
	int i ,i0;					//contador
	int inicial;				//bloco inicial
	int pos;					//posição inicial
	int flag;					//flag de teste
	int remanescente;			//quanto falta para ser gravado
	int capacity;				//capacidade do inodo
	int nblock;					//numero do novo bloco
	//lê o inodo:
	inode_load(inumber,&inode_block);
	//verifica se inodo é válido:
	if (!inode_block.isvalid){
		return 0;
	} 
	//verifica espaço disponível:
	if (length > espacoLivre() || length > (espacoLivre() - offset))
	{
		cout<<"tamanho maior que espaço livre \n livre: "<<espacoLivre()<<endl;
		//atualiza tamanho de acordo com espaço disponível:
		length = espacoLivre() - offset;
		cout<<"tamanho atualizado: "<<length<<endl;
	}
	cout<<"lendo inodo: "<<inumber<<endl;
	remanescente = length;
	cout<<"faltam gravar: "<<remanescente<<endl;
	capacity = inode_block.size;
	cout<<"capacidade: "<<capacity<<endl;

	//enquanto não hover espaço, será preciso "alocar":
	while (capacity < (length+offset)) {
		flag=0;
		cout<<"capacidade menor que o necessário"<<endl;
		nblock = blocoDisponivel();
		if (nblock == 0)
		{
			cout<<"nenhum bloco disponivel"<<endl;
			return 0;
		} 
		cout<<"bloco disponivel: "<<nblock<<endl;
		//percorro os ponteiros diretos que ele tem:
		for(i=0;i<POINTERS_PER_INODE;i++)
		{
			//se encontra um ponteiro livre:
			if(inode_block.direct[i]<=0)
			{
				//aponta para o bloco livre:
				inode_block.direct[i]=nblock;
				//salva ele no bitmap:
				bitmap[nblock]=true;
				//amplia a capacidade do inodo:
				capacity += DISK_BLOCK_SIZE;
				cout<<"bloco alocado: "<<nblock<<endl;
				cout<<"nova capacidade: "<<capacity<<endl;
				//sinaliza que alocou o suficiente:
				flag=1;
				//sai do for:
				break;
			}
		}
		//verifica se já tem a capacidade necessária:
		if (flag == 0){
			//só não entra aqui se não encontrou nenum ponteiro livre!
			//lê bloco indireto:
			disk_read(inode_block.indirect,indirect_block.data);
			//percorre o bloco indireto:
			for(i=0;i<POINTERS_PER_BLOCK;i++)
			{
				//se encontrar um ponteiro vazio:
				if (indirect_block.pointers[i] <= 0){
					//faz apontar para o novo bloco:
					indirect_block.pointers[i]=nblock;
					//salva ele no bitmap:
					bitmap[nblock] = true;
					//amplia a capacidade do inodo:
					capacity += DISK_BLOCK_SIZE;
					cout<<"bloco indiretamente alocado: "<<nblock<<endl;
					cout<<"nova capacidade: "<<capacity<<endl;
					//sai do for:
					break;
				}
			}
		}
	}
	//atualiza tamanho do inodo:
	inode_block.size=capacity;
	cout<<"fim da alocação \n nova capacidade: "<<capacity<<endl;
	//salva a informação do inodo:
	inode_save(inumber,inode_block);

	//encontra bloco onde inicia:
	i0 = offset/DISK_BLOCK_SIZE;
	//salva o inicial:
	inicial=i0;
	cout << "Primeiro bloco analisado: " << inode_block.direct[i0] << endl;
	//encontra posicao inicial:
	pos = offset%DISK_BLOCK_SIZE;
	//se for um direto:
	if (i0 < POINTERS_PER_INODE) {
		//percorre blocos diretos:
		for (i = i0; i < POINTERS_PER_INODE; i++){
			//verifica se é válido:
			if (inode_block.direct[i] > 0) 
			{
				//lê o bloco de dado:
				disk_read(inode_block.direct[i],data_block.data);
				//verifica se tudo já foi gravado:
				if (remanescente == 0) 
				{
					//atualiza tamanho do inodo:
					//inode_block.size=length;
					//grava o inodo:
					inode_save(inumber,inode_block);
					//retorna sucesso:
					return length;
				}
				cout << "escrevendo no bloco " << inode_block.direct[i] << endl;
				//se tudo cabe no bloco:
				if (remanescente <= DISK_BLOCK_SIZE)
				{
					cout<<"Cabe em um bloco"<<endl;
					//se for o primeiro bloco:
					if(i==inicial){
						//copia os dados considerando o offset:
						copy(data+pos, data+pos+remanescente , data_block.data+pos);
						cout<<"primeiro bloco copiado"<<endl;
					}
					//se não for o primeiro bloco:
					else
					{
						//copia os dados não considerando o offset:
						copy(data+pos, data+pos+remanescente , data_block.data);
						cout<<"bloco copiado"<<endl;
					}
					//grava bloco no disco:
					disk_write(inode_block.direct[i],data_block.data);
					cout<<"bloco gravado"<<endl;
					//atualiza bitmap:
					bitmap[inode_block.direct[i]] = true;
					//atualiza tamanho do inodo:
					//inode_block.size=length;
					//grava o inodo:
					//inode_save(inumber,inode_block);
					//retorna sucesso:
					return length;
				}
				//se for escrever em mais blocos:
				else
				{
					cout<<"Não cabe em um bloco"<<endl;
					//se for o primeiro bloco:
					if(i==inicial){
						//copia os dados considerando o offset:
						copy(data+pos, data+pos+remanescente , data_block.data+pos);
						cout<<"primeiro bloco copiado"<<endl;
						//atualiza remanescente considerando offset:
						//remanescente -= (DISK_BLOCK_SIZE-pos);
					}
					//se não for o primeiro bloco:
					else
					{
						//copia os dados não considerando o offset:
						copy(data+pos, data+pos+remanescente , data_block.data);
						cout<<"bloco copiado"<<endl;
						//atualiza remanescente não considerando offset:
						//remanescente -= DISK_BLOCK_SIZE;
					}
					//atualiza posicao:
					pos += DISK_BLOCK_SIZE;
					//atualiza remanescente não considerando offset:
				 	remanescente -= DISK_BLOCK_SIZE;
					//grava bloco no disco:
					disk_write(inode_block.direct[i],data_block.data);
					cout<<"bloco gravado"<<endl;
					//atualiza bitmap:
					bitmap[inode_block.direct[i]] = true;
				}
			}
		}
		//atualiza o ponto inicial para navegar pelos indiretos:
		i0 -= POINTERS_PER_INODE;
	}
	//agora percorre os indiretos, se o primeiro for o primeiro ele vem direto pra ca, se não só se não gravou tudo em indireto:
	for (i=i0; i < POINTERS_PER_INODE; i++){
		cout<<"entrando nos indiretos  "<<i0<<endl;
		//verifica se é válido:
		if (indirect_block.pointers[i] > 0)
		{
			//lê o bloco de dado:
			disk_read(indirect_block.pointers[i],data_block.data);
			//verifica se tudo já foi gravado:
			if (remanescente == 0) 
			{
				//grava o inodo:
				inode_save(inumber,inode_block);
				//retorna sucesso:
				return length;
			}
			//se tudo cabe no bloco:
			if (remanescente <= DISK_BLOCK_SIZE)
			{
				cout<<"Cabe em um bloco indireto"<<endl;
				//se for o primeiro bloco:
				if(i==inicial){
					//copia os dados considerando o offset:
					copy(data+pos, data+pos+remanescente , data_block.data+pos);
					cout<<"primeiro bloco copiado"<<endl;
				}
				//se não for o primeiro bloco:
				else
				{
					//copia os dados não considerando o offset:
					copy(data+pos, data+pos+remanescente , data_block.data);
					cout<<"bloco copiado"<<endl;
				}
				//grava bloco no disco:
				disk_write(inode_block.direct[i],data_block.data);
				cout<<"bloco copiado"<<endl;
				//atualiza bitmap:
				bitmap[inode_block.direct[i]] = true;
				//atualiza tamanho do inodo:
				//inode_block.size=length;
				//grava o inodo:
				inode_save(inumber,inode_block);
				//retorna sucesso:
				return length;
			}
			//se for escrever em mais blocos:
			else
			{
				cout<<"Não em um bloco cabe em um bloco indireto"<<endl;
				//se for o primeiro bloco:
				if(i==inicial){
					//copia os dados considerando o offset:
					copy(data+pos, data+pos+remanescente , data_block.data+pos);
					cout<<"primeiro bloco copiado"<<endl;
					//atualiza remanescente considerando offset:
					//remanescente -= (DISK_BLOCK_SIZE-pos);
				}
				//se não for o primeiro bloco:
				else
				{
					//copia os dados não considerando o offset:
					copy(data+pos, data+pos+remanescente , data_block.data);
					cout<<"bloco copiado"<<endl;
					//atualiza remanescente não considerando offset:
					//remanescente -= DISK_BLOCK_SIZE;
				}
				//atualiza posicao:
				pos += DISK_BLOCK_SIZE;
				//atualiza remanescente não considerando offset:
			 	remanescente -= DISK_BLOCK_SIZE;
				//grava bloco no disco:
				disk_write(inode_block.direct[i],data_block.data);
				cout<<"bloco copiado"<<endl;
				//atualiza bitmap:
				bitmap[inode_block.direct[i]] = true;
			}
		}
	}
	//retorna erro caso não tenha gravado o ultimo bloco:
	return 0;
}