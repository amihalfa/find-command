/**
 * HALFAOUI Amirouche
 * ABDOULHOUSSEN Hamza
 *
 * 30.06.2011
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <sys/time.h>
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <fnmatch.h>

#define	A_DAY	(long)(60*60*24)

/**
 * Differents types d'operations booleennes possibles entre les noeuds de l'arbre
 */
typedef enum Boolean_operation{
	AND, OR, COMA
} Boolean_operation;

/**
 * Types de parametres que prennent les predicats
 */
typedef enum Argument_type{
	NONE, STRING, INT, EXEC_ARGS
} Argument_type;

/**
 * Differents identifiants pour les predicats, optimisation des calculs
 */
typedef enum Predicate_id{
	TRUE = 0, FALSE = 1, NAME = 2, TYPE = 3, UID = 4, GID = 5, 
	USER = 6, GROUP = 7, ATIME = 8, CTIME = 9, MTIME = 10, 
	PERM = 11, EXEC = 12, PRINT = 13, LS = 14, PRUNE = 15, 
	P_AND = 16, P_OR = 17, L_PAREN = 18, R_PAREN = 19, P_COMA = 20,
	P_NOT = 21
} Predicate_id;

/**
 * Structure pour stocker un predicat et un pointeur vers le suivant
 */
typedef struct{
	Predicate_id		id;
	char 				*arg_char;
	char 				**arg_tab;
	int					arg_int;
	char				arg_opt;
} Predicate;


/**
 * Structure en arbre pour conserver la notion de parentheses et les predicats chainÃ©s
 */
typedef struct Predicates_tree Predicates_tree;
struct Predicates_tree {
	Boolean_operation 	boolean_operation;
	int					negation;
	Predicates_tree 	**children;
	Predicate 			predicate;
	int 				size;
};

/**
 * Structure contenant des donnees remplies par le parcours de l'arbre
 */
typedef struct {
	/* S'il est necessaire de rajouter le print, cette variable
	 doit valoir 1 et 0 sinon */
	int add_print;
} Extra_data;

/**
 * Structure permettant de gerer les options du programme
 */
typedef struct {
	int mindepth;
	int maxdepth;
	int sort;
} Find_options;

/**
 * Structure permettant de stocker toutes les informations 
 * relatives a un fichier
 */
typedef struct{
	struct dirent * f_dirent;
	struct stat * f_stat;
	char * f_path;
} File_definition;

/**
 * Permet de recuperer l'id et le type d'argument d'un predicat
 * @return 0 si predicat existant, 1 sinon
 */
int get_predicate_info( char *name, Predicate_id *id, Argument_type *type);

/**
 * Lecture de l'ensemble des predicats et stockage dans un arbre
 * @param tab 			Tableau de predicats
 * @param size 			Taille du tableau passe en parametre
 * @param pt 			Pointeur vers l'arbre de predicats a remplir
 * @param extra_data 	Pointeur vers la structure de donnees annexes a remplir
 */
void create_predicats_tree(char ** tab, int size, Predicates_tree *pt, Extra_data *extra_data);

/**
 * Fonction qui pour un chemin passe en parametre effectue l'ensemble des predicats
 * @param path			Chemin a analyser
 * @param predicates_tree Arbre contenant les predicats retenus
 * @param find_options	options du programme
 * @param depth			Profondeur dans la hierarchie
 */
void find( char * path,  Predicates_tree * predicates_tree, Find_options *find_options, int depth);


/**
 * Conversion d'une chaine en identifiant de type
 * @param type chaine a convertir
 * @return identifiant du type
 */
int string_to_type_id		(char * type);

/**
 * Conversion d'une chaine (nom d'utilisateur) en UID
 * @param username nom d'utilisateur
 * @return UID correspondant
 */
int string_to_user_id		(char * username);

/**
 * Conversion d'un nom de groupe en GID
 * @param groupname	nom de groupe
 * @return GID correspondant
 */
int string_to_group_id		(char * groupname);

/**
 * Conversion d'une chaine en entier
 * @param string		chaine a convertir
 * @param base			Base dans laquelle est ecrit le nombre
 * @return l'entier correspondant, -1 si erreur
 */
int string_to_integer		(char * string, int base);

/**
 * Indique le caractere decrivant le type de fichier passe en parametre
 * @param type_id Identifiant de type
 * @return le caractere correspondant au type passe en parametre
 */
char type_id_to_char		(int type_id);

/**
 * Comparateur de deux fichiers entre eux
 * Pour le sort, permet de les classer par ordre alphabetique
 */
int file_def_comparator		(const void * fd1, const void * fd2);

/**
 * Permet d'evaluer un predicat sur un fichier en particulier
 * @p file_dirent		dirent du fichier
 * @p file_stat			stat decrivant le inode du fichier
 * @p path				chemin du fichier
 * @p predicate 		predicat a evaluer
 * @p find_options		options du programme
 * @p depth				profondeur du fichier dans l'arborescence
 * @return 1 si reussite, 0 si echec
 */
int evaluate_predicate		(
								struct dirent *file_dirent, 
								struct stat *file_stat, 
								char * path,  
								Predicate *predicate, 
								Find_options * fo, 
								int depth
							);

/**
 * Permet d'evaluer un arbre de predicats sur un fichier en particulier
 * @p file_dirent		dirent du fichier
 * @p file_stat			stat decrivant le inode du fichier
 * @p path				chemin du fichier
 * @p predicate_tree 	arbres de predicats a evaluer
 * @p find_options		options du programme
 * @p depth				profondeur du fichier dans l'arborescence
 * @return 1 si reussite, 0 si echec
 */
int evaluate_predicate_tree	(
								struct dirent *file_dirent, 
								struct stat *file_stat, 
								char *path,  
								Predicates_tree *predicate_tree,
								Find_options * fo, 
								int depth
							);

/**
 * Evaluation de predicats particuliers sur des fichiers lorsqu'ils necessitent un 
 * certain nombre de lignes de code
 */
int predicate_perm_file		(struct stat* file_stat, Predicate *predicate);
int predicate_ls_file		(struct dirent *file_dirent, struct stat* file_stat, char *path);
int predicate_time_file		(long date, Predicate *predicate);
int predicate_exec_file		(char *path, Predicate *predicate);

/**
 * Gestionnaire d'erreurs
 * Quitte le programme brutalement avec le bon affichage
 * @p p_id 		Id du predicat appelant
 * @p p_val		Valeur du predicat
 * @p err_id	Identifiant de l'erreur
 */
void error_predicate_option	(Predicate_id p_id, char* p_val, int err_id);


/** 
 * Permet d'ajouter le print quand il faut 
 */
Predicates_tree * add_print_predicat(Predicates_tree *pt);

/** 
 * Nom du programme pour permettre l'affichage correct des erreurs 
 */
char * cmd_name;

/**
 * Programme principal
 * @param argc nombre d'elements de la ligne de commandes
 * @param argv arguments de la ligne de commandes
 */
int main(int argc, char ** argv){
	
	int i = 1;
	int paths_nb = 0, options_nb = 0;
	int all_options_explored = 0;
	Predicates_tree predicates_tree, *predicates_tree_pointer;
	Extra_data extra_data;
	Find_options find_options;
	
	/* Stockage du nom du programme */
	cmd_name = argv[0];
	
	/* Pre-remplissage du tableau de predicats */
	predicates_tree.boolean_operation = AND;
	predicates_tree.negation = 			0;
	predicates_tree.children = 			NULL;
	predicates_tree.size = 				0;
	predicates_tree.predicate.id = 		TRUE;
	
	predicates_tree_pointer = &predicates_tree;
	
	extra_data.add_print = 1;
	
	find_options.maxdepth =	-1;
	find_options.mindepth =	-1;
	find_options.sort =		0;
	
	/* calcul du nombre de dossiers a parcourir */
	while( !all_options_explored && i < argc ){
		if(!all_options_explored && argv[i][0] != '-' && argv[i][0] != '('){
			paths_nb++;
			i++;
		} else {
			all_options_explored = 1;
		}
	}

	/* Analyse des options du programme */
	all_options_explored = 0;
	while( !all_options_explored && i < argc ){
		
		/* Cas du mindepth */
		if(!strcmp(argv[i], "-mindepth")){
			if(i+1 == argc){
				error_predicate_option(AND, NULL, 1);
			}
			find_options.mindepth = string_to_integer(argv[i+1], 10);
			if(find_options.mindepth == -1)
				error_predicate_option(AND, argv[i+1], 1);
			
			options_nb+=2;
			i++;
			
		/* Cas du maxdepth */
		} else if(!strcmp(argv[i], "-maxdepth")){
			if(i+1 == argc){
				error_predicate_option(AND, NULL, 2);
			}
			find_options.maxdepth = string_to_integer(argv[i+1], 10);
			if(find_options.maxdepth == -1)
				error_predicate_option(AND, argv[i+1], 2);
			options_nb+=2;
			i++;
			
		/* Cas du sort */
		} else if(!strcmp(argv[i], "-sort")){
			find_options.sort = 1;
			options_nb++;
		} else {
			all_options_explored = 1;
		}
		i++;
	}
	
	/* Analyse et stockage des predicats */
	create_predicats_tree(argv + paths_nb + options_nb + 1, argc - paths_nb - options_nb - 1, predicates_tree_pointer, &extra_data);
	
	/* Ajout du predicat print si necessaire */
	if(extra_data.add_print == 1)
		predicates_tree_pointer = add_print_predicat(predicates_tree_pointer);

	/* Recherche sur les fichiers  */
	if(paths_nb == 0){
		find(".", predicates_tree_pointer, &find_options, 0);
	} else for( i = 0; i < paths_nb; i++){
		find(argv[i+1], predicates_tree_pointer, &find_options, 0);
	}	
	return 0;
}

void create_predicats_tree(char ** tab, int size, Predicates_tree * pt, Extra_data *extra_data){

	int nb_par = 0, i = 0, deb_par = -1, skip = 0, nb_or = 0;
	Boolean_operation last_boolean_operation = AND;
	int last_negation = 0;
	
	i=0;
	while(i<size){
		
		// Parenthese ouvrante, nouveau noeud de l'arbre
		if(tab[i][0] == '(' && tab[i][1] == '\0'){
			nb_par++;
			if(nb_par == 1){
				deb_par = i;
			}
			
		// Parenthese fermante, find du nouveau noeud
		} else if(tab[i][0] == ')' && tab[i][1] == '\0'){
			nb_par--;
			if(nb_par == 0){
				if(pt->size == 0){
					pt->children = 	(Predicates_tree**)malloc(sizeof(Predicates_tree*));
				} else {
					pt->children = 	(Predicates_tree**)realloc(pt->children,(pt->size+1)*sizeof(Predicates_tree*));
				}
				pt->children[pt->size] = 	(Predicates_tree*)malloc(sizeof(Predicates_tree));
				pt->size++;
				
				pt->children[pt->size-1]->boolean_operation = last_boolean_operation;
				pt->children[pt->size-1]->negation			= last_negation;
				pt->children[pt->size-1]->size = 				0;
				
				last_boolean_operation = AND;
				last_negation = 0;
				
				// Appel de la fonction sur le nouveau noeud
				create_predicats_tree(tab  + deb_par + 1, i - deb_par -1, pt->children[pt->size-1], extra_data);
			}
		} else if(nb_par == 0){
			if(skip == 0){
				/************************ Gestion des predicats purs ***************************/
				Predicate_id id, next_id;
				Argument_type arg_type;
				Predicate * temp_predicate;
				
				get_predicate_info(tab[i], &id, &arg_type);
				
				if(arg_type != NONE){
					skip++;
					if(arg_type == EXEC_ARGS){
						while(i+skip < size && strcmp(tab[i+skip], ";")){
							skip++;
						}
					}
				}
				
				// Cas de l'overflow
				if(i+skip >= size){
					error_predicate_option	(id, NULL, 0);
				}
				
				/*
				 * Gestion separee des OR car priorite du AND
				 * Nouveau noeud
				 */
				if(id == P_OR){
					if(i == 0){
						fprintf(stderr,"%s: invalid predicate: -o\n",cmd_name);
						fprintf(stderr,"%s: invalid expression\n",cmd_name);
						exit(1);
					}
 					nb_or = i;
				} 
				if(nb_or > 0){
					if(i+skip+1 < size){
						get_predicate_info(tab[i+1+skip], &next_id, &arg_type);
					} else {
						next_id = R_PAREN;
					}
					
					// Recherche de la fin de la portee du OR
					if(next_id == L_PAREN && (i == nb_or || last_negation == 1)){
						last_boolean_operation = OR;
						nb_or = 0;
					} 
					else if(next_id == P_NOT && i == nb_or){
						last_negation = 1;
					}
					else if(
						next_id == L_PAREN 
						|| next_id == R_PAREN 
						|| next_id == P_OR 
						|| next_id == P_COMA 
						|| (next_id == P_NOT && id == P_AND)
					){
						if(pt->size == 0){
							pt->children = 	(Predicates_tree**)malloc(sizeof(Predicates_tree*));
						} else {
							pt->children = 	(Predicates_tree**)realloc(pt->children,(pt->size+1)*sizeof(Predicates_tree*));
						}
						pt->children[pt->size] = 	(Predicates_tree*)malloc(sizeof(Predicates_tree));
						pt->size++;
						
						pt->children[pt->size-1]->boolean_operation = 	OR;
						pt->children[pt->size-1]->size = 				0;
						
						create_predicats_tree(tab + nb_or + 1, i + skip - nb_or, pt->children[pt->size-1], extra_data);
						nb_or = 0;
					}
					
				} else {
					
					// Cas normal , evalutation de predicat
					if(id != P_AND && id != P_COMA && id != P_NOT){
						if(pt->size == 0){
							pt->children = 				(Predicates_tree**)malloc(sizeof(Predicates_tree*));
						} else {
							pt->children = 				(Predicates_tree**)realloc(pt->children,(pt->size+1)*sizeof(Predicates_tree*));
						}
						pt->children[pt->size] = 	(Predicates_tree*)malloc(sizeof(Predicates_tree));
						pt->size++;
						
						pt->children[pt->size-1]->size = 				0;
						pt->children[pt->size-1]->boolean_operation = 	last_boolean_operation;
						pt->children[pt->size-1]->negation = 			last_negation;
						last_boolean_operation = AND;
						last_negation = 0;
						
						temp_predicate = 								&(pt->children[pt->size-1]->predicate);
						temp_predicate->id = 							id;
						
						// Pour les 3 actions suivantes, pas de print a ajouter
						switch(id){
							case PRINT:
							case EXEC:
							case LS:
								extra_data->add_print = 0;
						}
						
						// Analyse et conversion des parametres
						switch(arg_type){
							case STRING:
								temp_predicate->arg_char = tab[i+1];
								break;
							case INT:
								switch(id){
									case TYPE:
										temp_predicate->arg_int = string_to_type_id(tab[i+1]);
										break;
									case USER:
										temp_predicate->arg_int = string_to_user_id(tab[i+1]);
										break;
									case GROUP:
										temp_predicate->arg_int = string_to_group_id(tab[i+1]);
										break;
									case PERM:
										if(tab[i+1][0] == '/' || tab[i+1][0] == '-'){
											temp_predicate->arg_opt = tab[i+1][0];
											temp_predicate->arg_int = string_to_integer(tab[i+1]+1, 8);
										} else {
											temp_predicate->arg_opt = ' ';
											temp_predicate->arg_int = string_to_integer(tab[i+1], 8);
										}
										break;
									case ATIME:
									case CTIME:
									case MTIME:
										if(tab[i+1][0] == '+' || tab[i+1][0] == '-'){
											temp_predicate->arg_opt = tab[i+1][0];
											temp_predicate->arg_int = string_to_integer(tab[i+1]+1, 10);
										} else {
											temp_predicate->arg_opt = ' ';
											temp_predicate->arg_int = string_to_integer(tab[i+1], 10);
										}
										if(temp_predicate->arg_int == -1)
											error_predicate_option(id, tab[i+1], 0);
										break;
									default:
										temp_predicate->arg_int = string_to_integer(tab[i+1], 10);
										if(temp_predicate->arg_int == -1)
											error_predicate_option(id, tab[i+1], 0);
										break;
								}
								break;
							case EXEC_ARGS:
								temp_predicate->arg_int = skip-1;
								temp_predicate->arg_tab = &tab[i+1];
								break;
							default:
								break;
						}
						
					} else {
						if(id == P_AND)
							last_boolean_operation = AND;
						else if(id == P_COMA)
							last_boolean_operation = COMA;
						else if(id == P_NOT)
							last_negation = 1;
					}
					
				}
				
			} else {
				skip--;
			}
		}
		i++;
	}
}

int get_predicate_info( char *name, Predicate_id *id, Argument_type *type){
	
	int i;
 	static const char predicates_names[][10] = {
		"-true",	"-false",	"-name",	"-type",	"-uid",		"-gid",		"-user",	"-group", 
		"-atime", 	"-ctime", 	"-mtime", 	"-perm", 	"-exec", 	"-print", 	"-ls", 		"-prune",
		"-a",		"-o",		"(",		")",		",",		"-not"
	};
	static const Argument_type predicates_args[] = 	{
		NONE, 		NONE, 		STRING,		INT,		INT,		INT,		INT,		INT,
		INT,		INT,		INT,		INT,		EXEC_ARGS,	NONE,		NONE,		NONE,
		NONE,		NONE,		NONE,		NONE,		NONE,		NONE
	};
	
	for(i = 0; i < 22; i++){
		if(!strcmp(predicates_names[i], name)){
			*id 	= i;
			*type	= predicates_args[i];
			return 0;
		}
	}
	fprintf(stderr,"%s: invalid predicate: %s\n",cmd_name, name);
	fprintf(stderr,"%s: invalid expression\n",cmd_name);
	exit(1);
}

/* Fonction permettant l'execution de la commande find */
void find( char * path,  Predicates_tree * predicates_tree, Find_options *find_options, int depth){
	
	const int PATH_LENGTH = 255 + strlen(path);
	
	char complete_path	[PATH_LENGTH];
	char complete_path2	[PATH_LENGTH];
	char *file_path;
	File_definition * file_definitions = (File_definition *)malloc(sizeof(File_definition));
	int file_definitions_size = 0, i;
	
	struct dirent *current;
	struct stat file_stat;
    DIR *rep;
	strcpy(complete_path, path);
	rep = opendir(path);
	
	// Fichier inexistant
	if(rep == NULL){
		fprintf(stderr,"%s: `%s': %s\n",cmd_name, path, strerror(errno));
		return;
	}
	
	if(depth > 0){
		
		while ((current = readdir(rep))) {	
			if(strcmp(current->d_name, ".") && strcmp(current->d_name, "..")){			
				
				if(find_options->mindepth <= depth){
					file_path = (char*)malloc(PATH_LENGTH*sizeof(char));
					strcpy(file_path, path);
					strcat(file_path, "/");
					strcat(file_path, current->d_name);
					stat(file_path, &file_stat);
					
					/* Evaluation des predicats sur le fichier en cours */
					if(find_options->sort){
						file_definitions_size++;
						file_definitions = (File_definition*)realloc(file_definitions, file_definitions_size * sizeof(File_definition));
						file_definitions[file_definitions_size-1].f_dirent = current;
						file_definitions[file_definitions_size-1].f_stat = (struct stat*)malloc(sizeof(struct stat));
						memcpy(file_definitions[file_definitions_size-1].f_stat, &file_stat, sizeof(struct stat));
						file_definitions[file_definitions_size-1].f_path = file_path;
					} else 
						if(find_options->mindepth <= depth)
							evaluate_predicate_tree(current, &file_stat, file_path, predicates_tree, find_options, depth);
				}
				
				/*ecriture du chemin complet dans complete_path2 */
				if(!find_options->sort && current->d_type == DT_DIR && (find_options->maxdepth == -1 || find_options->maxdepth >= depth+1)){
					strcpy(complete_path2, complete_path);
					strcat(complete_path2, "/");
					strcat(complete_path2, current->d_name);	
					/*recursivite*/
					find(complete_path2, predicates_tree, find_options, depth+1);
				}
			}
		}
	} else {
		
		while ((current = readdir(rep))) {	
			if(!strcmp(current->d_name, ".")){
				stat(path, &file_stat);
				
				/* Si le tri est demande, on prepare les fichiers pour un futur appel */
				if(find_options->sort){
					file_definitions_size++;
					file_definitions = (File_definition*)realloc(file_definitions, file_definitions_size * sizeof(File_definition));
					file_definitions[file_definitions_size-1].f_dirent = current;
					file_definitions[file_definitions_size-1].f_stat = (struct stat*)malloc(sizeof(struct stat));
					memcpy(file_definitions[file_definitions_size-1].f_stat, &file_stat, sizeof(struct stat));
					file_definitions[file_definitions_size-1].f_path = path;
				} else {
					if(find_options->mindepth <= depth)
						evaluate_predicate_tree(current, &file_stat, path, predicates_tree, find_options, depth);
					
					if(find_options->maxdepth == -1 || find_options->maxdepth >= depth+1)
						find(path, predicates_tree, find_options, depth+1);
				}
			}
		}
		
	}
	
	// Cas ou le sort est actif
	// On prepare les appels mais on ne les fait qu'apres le tri
	qsort(file_definitions, file_definitions_size, sizeof(File_definition), file_def_comparator);
	
	if(find_options->sort){
		for(i=0; i<file_definitions_size; i++){
			File_definition *f = file_definitions+i;
			if(depth == 0){
				if(find_options->mindepth <= depth)
					evaluate_predicate_tree(f->f_dirent, f->f_stat, f->f_path, predicates_tree, find_options, depth);
				
				find(f->f_path, predicates_tree, find_options, depth+1);
			} else {
				if(find_options->mindepth <= depth)
					evaluate_predicate_tree(f->f_dirent, f->f_stat, f->f_path, predicates_tree, find_options, depth);
				
				if(f->f_dirent->d_type == DT_DIR && (find_options->maxdepth == -1 || find_options->maxdepth >= depth+1)){
					/*recursivite*/
					find(f->f_path, predicates_tree, find_options, depth+1);
				}
			}
		}
	}
	
	closedir(rep);
}

int evaluate_predicate_tree
(
	struct dirent *file_dirent, 
	struct stat *file_stat, 
	char *path, 
	Predicates_tree *predicate_tree, 
	Find_options * fo, 
	int depth
){
	int ok = 1;
	if(predicate_tree->size == 0){
		ok = evaluate_predicate(file_dirent, file_stat, path, &predicate_tree->predicate, fo, depth);
		
	} else {
		int i = 0;
		for(i = 0; i < predicate_tree->size; i++){
			
			// Evaluation du OR
			if(predicate_tree->children[i]->boolean_operation == OR){
				ok = ok || evaluate_predicate_tree(file_dirent, file_stat, path, predicate_tree->children[i], fo, depth);
				
			// Evaluation avec AND
			} else if(predicate_tree->children[i]->boolean_operation == AND){
				ok = ok && evaluate_predicate_tree(file_dirent, file_stat, path, predicate_tree->children[i], fo, depth);
			
			// Cas a part
			} else {
				ok = evaluate_predicate_tree(file_dirent, file_stat, path, predicate_tree->children[i], fo, depth);
			}
		}
	}
	
	// Gestion du NOT
	if(predicate_tree->negation) return !ok;
	return ok;
}


int evaluate_predicate
(
	struct dirent *file_dirent, 
	struct stat* file_stat,  
	char *path, 
	Predicate *predicate, 
	Find_options * fo, 
	int depth
){
	time_t now;
	time(&now);
	switch(predicate->id){
		
		case TRUE:
			return 1;
		case FALSE:
			return 0;
		case NAME:
			return fnmatch(predicate->arg_char, file_dirent->d_name, FNM_NOESCAPE) == 0;
		case TYPE:
			return file_dirent->d_type == predicate->arg_int;
		case UID:
		case USER:
			return file_stat->st_uid == predicate->arg_int;
		case GID:
		case GROUP:
			return file_stat->st_gid == predicate->arg_int;
		case ATIME:
			return predicate_time_file((now-file_stat->st_atim.tv_sec) / A_DAY, predicate);
		case CTIME:
			return predicate_time_file((now-file_stat->st_ctim.tv_sec) / A_DAY, predicate);
		case MTIME:
			return predicate_time_file((now-file_stat->st_mtim.tv_sec) / A_DAY, predicate);
		case PERM:
			return predicate_perm_file(file_stat, predicate);
			break;
		case EXEC:
			return predicate_exec_file(path, predicate);
		case PRINT:
			printf("%s\n", path);
			return 1;
		case LS:
			return predicate_ls_file(file_dirent, file_stat, path);
		case PRUNE:
			return ( (fo->mindepth == -1 && depth == 0) || (fo->mindepth != -1 && fo->mindepth == depth));
		default :
			return 1;
	}
}

int predicate_perm_file(struct stat* file_stat, Predicate *predicate){
	switch(predicate->arg_opt){
		case '/':
			return predicate->arg_int & (file_stat->st_mode & 511);
		case '-':
			return (predicate->arg_int & (file_stat->st_mode & 511)) == predicate->arg_int;
		default:
			return predicate->arg_int == (file_stat->st_mode & 511);
	}
}

int predicate_time_file	(long date, Predicate *predicate){
	switch(predicate->arg_opt){
		case '+':
			return date > predicate->arg_int;
		case '-':
			return date < predicate->arg_int;
		default:
			return date == predicate->arg_int;
	}
}

int predicate_ls_file(struct dirent *file_dirent, struct stat* file_stat, char *path){
	int i;
	char date_string[17];
	char sym_link[256];
	struct passwd *p = getpwuid(file_stat->st_uid);
	struct group *g = getgrgid(file_stat->st_gid);
	struct tm * modif_time = localtime(&file_stat->st_mtim.tv_sec);
	strftime(date_string, 17, "%Y-%m-%d %H:%M", modif_time);
	
	printf("%6d %4d %c", (int)file_stat->st_ino, (int)(file_stat->st_blocks/2), type_id_to_char(file_dirent->d_type));
	
	/* Ecriture des permissions */
	for(i = 256; i >= 1; i/=8){
		printf("%c", file_stat->st_mode & (i) ? 'r' : '-');
		printf("%c", file_stat->st_mode & (i/2) ? 'w' : '-');
		printf("%c", file_stat->st_mode & (i/4) ? 'x' : '-');	
	}
	
	printf(" %3d ", (int)file_stat->st_nlink);
	
	if(p != NULL)
		printf("%-8s ", p->pw_name);
	else
		printf("%-8d", file_stat->st_uid);
	
	if(g != NULL)
		printf("%-8s", g->gr_name);
	else
		printf("%-8d", file_stat->st_gid);
	
	printf(" %8d %s %s", (int)file_stat->st_size, date_string, path);
	
	// Cas des liens symboliques
	if(file_dirent->d_type == DT_LNK){
		int l = readlink(path, sym_link, sizeof(sym_link));
		sym_link[l] = '\0';
		printf(" -> %s\n", sym_link);
	} else {
		printf("\n");
	}
	return 1;
}

int predicate_exec_file	(char *path, Predicate *predicate){
	
	char ** argv = (char**) malloc((predicate->arg_int + 1)* sizeof(char *));
	pid_t child = fork();
	int status, i;
	
	for(i = 0; i < predicate->arg_int; i++)
		if(strcmp(predicate->arg_tab[i], "{}") == 0)
			argv[i] = path;
		else
			argv[i] = predicate->arg_tab[i];

	argv[predicate->arg_int] =  0;
	
	if(child == -1)
		exit(1);
	
	if(child != 0){
		int r;
		do
			r = wait(&status);
		while(r != child);
	} else {
		if(execvp(argv[0], argv) == -1)
			exit(1);
	}
	return !WEXITSTATUS(status);
}

int string_to_type_id(char * type){
	if(strlen(type) != 1){
		error_predicate_option(TYPE, type, 0);
	}
	switch(type[0]){
		case 'd' :
			return DT_DIR;
		case 'f' :
			return DT_REG;
		case 'l':
			return DT_LNK;
		case 's':
			return DT_SOCK;
		case 'b':
			return DT_BLK;
		case 'c':
			return DT_CHR;
		case 'p':
			return DT_FIFO;
		default:
			error_predicate_option(TYPE, type, 0);
	}
}

int string_to_user_id(char * username){
	struct passwd *p = getpwnam(username);
	if(p != NULL){
		return p->pw_uid;
	} else {
		error_predicate_option(USER, username, 0);
	}
}

int string_to_group_id(char * groupname){
	struct group *g = getgrnam(groupname);
	if(g != NULL){
		return g->gr_gid;
	} else {
		error_predicate_option(GROUP, groupname, 0);
		return 0;
	}
}

int string_to_integer(char * string, int base){
	char * first_invalid_char;
	long value = strtol(string, &first_invalid_char, base);
	if(first_invalid_char[0] == '\0' && string[0] != '\0'){
		return value;
	} else {
		return -1;
	}
}

char type_id_to_char(int type_id){
	switch(type_id){
		case DT_REG:
			return '-';
		case DT_DIR:
			return 'd';
		case DT_LNK:
			return 'l';
		case DT_SOCK:
			return 's';
		case DT_BLK:
			return 'b';
		case DT_CHR:
			return 'c';
		case DT_FIFO:
			return 'f';
		default:
			return ' ';
	}
}

Predicates_tree * add_print_predicat(Predicates_tree *pt){
	
	Predicates_tree * root = (Predicates_tree * ) malloc(sizeof(Predicates_tree));
	Predicates_tree * print = (Predicates_tree * ) malloc(sizeof(Predicates_tree));

	root->size = 				2;
	root->boolean_operation = 	AND;
	root->negation = 			0;
	
	print->size =				0;
	print->boolean_operation =	AND;
	print->negation =			0;
	print->predicate.id =		PRINT;
	
	root->children = (Predicates_tree**) malloc(2 * sizeof(Predicates_tree*));
	root->children[0] = pt;
	root->children[1] = print;
	
	return root;
	
}

int file_def_comparator(const void * fd1, const void * fd2){
	File_definition *f1 = (File_definition*) fd1;
	File_definition *f2 = (File_definition*) fd2;
	return strcmp( f1->f_path, f2->f_path);
}

void error_predicate_option	(Predicate_id p_id, char* p_val, int err_id){
	
	char time_type = ' ';
	char opt[20];
	if(err_id > 0){
		switch(err_id){
			case 1:
				strcpy(opt, "minddepth");
				break;
			case 2:
				strcpy(opt, "maxdepth");
				break;
			default:
				break;
		}
		if(p_val != NULL)
			fprintf(stderr,"%s: invalid argument for option [%s] (%s)\n",cmd_name, opt, p_val);
		else 
			fprintf(stderr,"%s: missing argument for option [%s]\n",cmd_name, opt);
		exit(1);
	}
	
	if(p_val == NULL){
		switch(p_id){
			case EXEC:
				fprintf(stderr,"%s: -exec: unterminated command\n",cmd_name);
				break;
			case NAME:
				fprintf(stderr,"%s: -name: missing pattern\n",cmd_name);
				break;
			case TYPE:
				fprintf(stderr,"%s: -type: missing file type\n",cmd_name);
				break;
			case UID:
				fprintf(stderr,"%s: -uid: missing UID\n",cmd_name);
				break;
			case GID:
				fprintf(stderr,"%s: -gid: missing GID\n",cmd_name);
				break;
			case USER:
				fprintf(stderr,"%s: -user: missing user name\n",cmd_name);
				break;
			case GROUP:
				fprintf(stderr,"%s: -group: missing group name\n",cmd_name);
				break;
			case PERM:
				fprintf(stderr,"%s: -perm: missing permission\n",cmd_name);
				break;
			case ATIME:
				time_type = 'a';
				break;
			case CTIME:
				time_type = 'c';
				break;
			case MTIME:
				time_type = 'm';
				break;	
		}
		if(time_type != ' '){
			fprintf(stderr,"%s: -time[%c]: missing parameter\n",cmd_name, time_type);
		}
	} else {
		switch(p_id){
			case TYPE:
				fprintf(stderr,"%s: -type: invalid file type: %s\n",cmd_name, p_val);
				break;
			case UID:
				fprintf(stderr,"%s: -uid: empty or invalid UID\n",cmd_name);
				break;
			case GID:
				fprintf(stderr,"%s: -gid: empty or invalid GID\n",cmd_name);
				break;
			case USER:
				fprintf(stderr,"%s: -user: cannot find user: %s\n",cmd_name, p_val);
				break;
			case GROUP:
				fprintf(stderr,"%s: -group: cannot find group: %s\n",cmd_name, p_val);
				break;
			case ATIME:
				time_type = 'a';
				break;
			case CTIME:
				time_type = 'c';
				break;
			case MTIME:
				time_type = 'm';
				break;	
		}
		if(time_type != ' '){
			fprintf(stderr,"%s: -time[%c]: invalid time spec\n",cmd_name, time_type);
		}
	}
	
	fprintf(stderr,"%s: invalid expression\n",cmd_name);
	exit(1);
}
