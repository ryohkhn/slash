#include "lineTreatment.h"

void perror_exit(char* msg){
    perror(msg);
    exit(1);
}

/*todo Pour éviter d'éventuels débordements (notamment lors de l'expansion des jokers),
 * on pourra limiter le nombre et la taille des arguments autorisés sur une ligne de commande slash :
 * #define MAX_ARGS_NUMBER 4096
 * #define MAX_ARGS_STRLEN 4096
 */
// fonction qui renvoie la position de la premiere asterisk dans le string, ou alors -1 s'il n'y en a pas
int getAsteriskPos(char * asteriskString){
    for(int index = 0; index < strlen(asteriskString); index++){
        if(asteriskString[index] == '*') return index;
    }
    return -1;
}
char * getPrefixe(unsigned long pos, char * asteriskString){
    char * prefixe = malloc(sizeof(char) * (pos + 1));
    testMalloc(prefixe);
    if(pos > 0){
        memcpy(prefixe,asteriskString,pos);
    }
    prefixe[pos] = '\0';
    return prefixe;
}

char * getSuffixe(unsigned long pos, char * asteriskString){
    size_t len = strlen(asteriskString);
    size_t sub_len = len - pos;
    char *sub = malloc(sub_len + 1);
    testMalloc(sub);
    strncpy(sub, asteriskString + pos, sub_len);
    sub[sub_len] = '\0';
    return sub;
}


char * getAstefixe(int start, const char * asteriskString){
    int end = start;
    while(asteriskString[end] != '\0' && asteriskString[end] != '/' ) end++;
    char * asterisk = malloc(sizeof(char) * (end - start + 1));
    testMalloc(asterisk);
    memcpy(asterisk,asteriskString + start,end - start);
    asterisk[end - start] = '\0';
    return asterisk;
}

// vérifie que asterisk_string est suffixe de entry_name
int strstrSuffixe(char * entry_name, char * asterisk_string){
    if(strlen(entry_name) < strlen(asterisk_string)) return 0;
    int index = strlen(entry_name) - 1;
    for(int i = strlen(asterisk_string) - 1; i >= 0; i--){
        if(entry_name[index] != asterisk_string[i]) return 0;
        index--;
    }
    return 1;
}


char ** replaceAsterisk(char * asteriskString) {
    //printf("--------------------- replaceAsterisk ---------------------------- \n");
    char ** res = malloc(sizeof(char *) * 1);
    res[0] = NULL;
    int posAsterisk = getAsteriskPos(asteriskString);


    // cas ou il n'y a pas d'asterisk
    if (posAsterisk == -1){
        struct stat st;
        // on vérifie que le chemin existe
        if(lstat(asteriskString,&st) == 0){ // si oui, on va a end
            goto endWithAsterisk;
        }else{
            return res; // sinon on renvoie res (vide)
        }
    }


    // cas ou l'asterisk n'est pas préfixe
    if((posAsterisk != 0 && asteriskString[posAsterisk - 1] != '/')) goto endWithAsterisk;

    /*on cherche à récupérer 3 char * correspondants à
     * la partie avant *
     * la partie ou il y a * (mais sans l'*)
     * la partie apres *
     */
    char *prefixe = getPrefixe(posAsterisk, asteriskString);
    char *asterisk = getAstefixe(posAsterisk + 1, asteriskString);
    unsigned long tailleSuf = strlen(prefixe) + strlen(asterisk) + 1;
    char *suffixe = getSuffixe(tailleSuf, asteriskString);


    //desormais on va ouvrir le repertoire préfixe
    DIR *dir = NULL;
    struct dirent *entry;

    if (strlen(prefixe) == 0) {
        dir = opendir(".");
    } else {
        dir = opendir(prefixe);
    }

    // On vérifie que le repertoire a bien été ouvert.
    // S'il n'est pas ouvert, c'est que le chemin n'est pas valide
    if (dir == NULL) {

        free(prefixe);
        free(suffixe);
        free(asterisk);

        return res;
    }

    // on parcourt les entrées du répertoire
    while ((entry = readdir(dir)) != NULL) {
        // cas où le fichier est '.', '..' ou caché
        if (entry->d_name[0] == '.') continue;
        // cas ou il y a une suite à la sous chaine avec asterisk (*/test par exemple)
        if (strlen(suffixe) != 0) {
            // dans ce cas, si le fichier n'est PAS un répertoire, on passe au suivant
            if (entry->d_type != DT_DIR && entry->d_type != DT_LNK) continue;
        }
        // à chaque correspondance entre le substring et entry, on realloc le tableau
        //
        if (strstrSuffixe(entry->d_name, asterisk) == 1) {
            char *newString = malloc(sizeof(char) * (strlen(prefixe) + strlen(entry->d_name) + strlen(suffixe) + 1));
            testMalloc(newString);
            sprintf(newString, "%s%s%s", prefixe, entry->d_name, suffixe);
            char** tmp = copyStringArray(res);
            char** replace = replaceAsterisk(newString);
            freeArray(res);
            res = combine_char_array(tmp, replace);
            freeArray(replace);
            freeArray(tmp);
            free(newString);
        }
    }
    closedir(dir);

    free(prefixe);
    free(suffixe);
    free(asterisk);

    //printf("sortie normale replaceAsterisk \n");
    //print_char_double_ptr(res);
    return res;

    // cas où l'on doit retourner le chemin donné en argument, seul et inchangé
    endWithAsterisk:
    freeArray(res);
    res = malloc(sizeof(char *) * 2);
    res[0] = malloc(sizeof(char) * strlen(asteriskString) + 1);
    strcpy(res[0], asteriskString);
    res[1] = NULL;

    //printf("sortie end replaceAsterisk \n");
    //print_char_double_ptr(res);
    return res;

}


char ** doubleAsteriskParcours(char * path, char * suffixe){
    //printf("------------------DoubleAsteriskParcours------------------\n");

    char **res = malloc(sizeof(char *) * 1);
    res[0] = NULL;

    DIR *dir = NULL;
    struct dirent *entry;
    if (strcmp(path, "") == 0) dir = opendir(".");
    else dir = opendir(path);
    //printf("opendir\n");

    // on parcourt le repertoire courant
    while ((entry = readdir(dir)) != NULL) {
        //printf("entry = %s\n",entry->d_name);


        // cas où l'entrée est '.', '..' ou cachée
        if (entry->d_name[0] == '.') continue;

        // cas où l'entrée est un repertoire
        if(entry->d_type == DT_DIR){
            //printf("entry = repertory\n");
            // on appelle replaceAsterisk avec path/completeSuffixe
            // si le chemin n'existe pas, cet appel renverra un char ** vide
            // sinon, il renverra les chemins possibles
            char * newString = malloc(sizeof(char) * (strlen(path) + strlen(entry->d_name) + strlen(suffixe) + 3));
            testMalloc(newString);
            sprintf(newString, "%s/%s/%s", path, entry->d_name, suffixe);
            char ** tmp = copyStringArray(res);
            char ** replace = replaceAsterisk(newString);
            freeArray(res);
            res = combine_char_array(tmp, replace);
            freeArray(replace);
            freeArray(tmp);
            free(newString);
            //printf("sortie appel replaceAsterisk\n");

            // maintenant on fait l'appel récursif,
            // pour tester si un autre chemin n'existe pas plus loin dans l'arborescence
            char * newPath;
            if(strcmp(path, "") == 0){
                newPath = malloc(sizeof(char) * (strlen(entry->d_name) + 1));
                sprintf(newPath,"%s",entry->d_name);
            }else{
                newPath = malloc(sizeof(char) * (strlen(path) + strlen(entry->d_name) + 2));
                sprintf(newPath,"%s/%s",path,entry->d_name);
            }
            tmp = copyStringArray(res);
            replace = doubleAsteriskParcours(newPath, suffixe);
            freeArray(res);
            res = combine_char_array(tmp, replace);
            freeArray(replace);
            freeArray(tmp);
            free(newPath);
            //printf("sortie appel res doubleAsteriskParcours\n");


        }
    }


    //print_char_double_ptr(res);
    return res;
}

/*
char ** doubleAsteriskParcours(char * path,char * suffixe, char * shortSuffixe){
    printf("------------------DoubleAsteriskParcours------------------\n");

    char ** res = malloc(sizeof(char *) * 1);
    res[0] = NULL;

    DIR * dir = NULL;
    struct dirent *entry;
    if(strcmp(path, "") == 0) dir = opendir(".");
    else dir = opendir(path);
    printf("opendir\n");
    while((entry = readdir(dir)) != NULL){
        printf("entry = %s\n",entry->d_name);
        // cas où l'entrée est '.', '..' ou cachée
        if (entry->d_name[0] == '.') continue;
        // cas où l'entrée n'est pas un répertoire et qu'on est deja rentré dans un repertoire dans l'appel précédent
        if (entry->d_type != DT_DIR) {
            printf("fichier\n");
            if(strcmp(path,"") != 0) {
                printf("path not \"\"\n");
                // cas etoile ?
                if (suffixe[0] == '*') {
                    // on commence par isoler le suffixe du suffixe (apres *)
                    char *endSuffixe = malloc(sizeof(char) * strlen(suffixe));
                    endSuffixe = getSuffixe(1, suffixe);
                    endSuffixe[strlen(suffixe) - 1] = '\0';

                    //on va comparer les entrées (fichiers) avec endSuffixe
                    if (strstrSuffixe(entry->d_name, endSuffixe) == 1) {
                        printf("correspondance fic / suffixe (avec *)\n");

                        char **add = malloc(sizeof(char *) * 2);
                        add[0] = malloc(sizeof(char *) * (strlen(path) + strlen(entry->d_name) + 2));
                        sprintf(add[0], "%s/%s", path, entry->d_name);
                        add[1] = NULL;
                        char **tmp = copyStringArray(res);
                        freeArray(res);
                        res = combine_char_array(tmp, add);
                        freeArray(add);
                        freeArray(tmp);
                    }
                    free(endSuffixe);
                } else {
                    if (strcmp(entry->d_name, suffixe) == 0) {
                        printf("correspondance fic / suffixe\n");

                        char **add = malloc(sizeof(char *) * 2);
                        add[0] = malloc(sizeof(char *) * (strlen(path) + strlen(suffixe) + 2));
                        sprintf(add[0], "%s/%s", path, suffixe);
                        add[1] = NULL;
                        char **tmp = copyStringArray(res);
                        freeArray(res);
                        res = combine_char_array(tmp, add);
                        freeArray(add);
                        freeArray(tmp);
                    } else {
                        printf("pas de correspondance fic / suffixe\n");
                    }
                }
            }
        }else{         // on fait l'appel récursif sur tous les repertoires
            printf("repertoire\n");
            char * newPath;
            if(strcmp(path, "") == 0){
                newPath = malloc(sizeof(char) * (strlen(entry->d_name) + 1));
                sprintf(newPath,"%s",entry->d_name);
            }else{
                newPath = malloc(sizeof(char) * (strlen(path) + strlen(entry->d_name) + 2));
                sprintf(newPath,"%s/%s",path,entry->d_name);
            }

            char** tmp = copyStringArray(res);
            printf("appel rec DAP avec newPath = %s, suffixe = %s, shortSuffixe = %s\n",newPath,suffixe, shortSuffixe);
            char ** replace = doubleAsteriskParcours(newPath,suffixe,shortSuffixe);
            freeArray(res);
            res = combine_char_array(tmp, replace);
            freeArray(replace);
            freeArray(tmp);
        }

    }
    printf("fin readdir\n");
    closedir(dir);
    return res;

}
*/

char ** replaceDoubleAsterisk(char * asteriskString){
    //printf("--------------------------------------- replaceDoubleAsterisk ------------------------------------------------\n");
    char ** res = malloc(sizeof(char *) * 1);
    res[0] = NULL;
    char * suffixe = getSuffixe(3, asteriskString);
    //printf("suffixe = %s\n",suffixe);
    char** tmp = copyStringArray(res);
    freeArray(res);
    char** replace = doubleAsteriskParcours("", suffixe);
    res = combine_char_array(tmp, replace);
    freeArray(tmp);
    freeArray(replace);
    return res;
}


char* supprimer_occurences_slash(const char *s){
    char *result = malloc(strlen(s) + 1);
    int isPreviousSlash = 0;
    int j = 0;
    for(int i = 0; s[i] != '\0'; i++) {
        if(s[i] == '/'){
            if(isPreviousSlash == 0){
                isPreviousSlash = 1;
                result[j] = s[i];
                j++;
            }
        }else{
            isPreviousSlash = 0;
            result[j] = s[i];
            j++;
        }


    }
    result[j] = '\0';
    return result;
}

static int saved[3]={-1,-1,-1};
static int duped[3]={0,1,2};

void reset_redi(){
    for(int i=0; i<3; i++){
        if(saved[i]!=-1)
            dup2(saved[i],duped[i]);
        close(saved[i]);
        saved[i]=-1;
    }
}

void exec_command_redirection(cmd_struct list){
    if(strcmp(*list.cmd_array,"cd")==0){
        process_cd_call(list);
    }
    else if(strcmp(*list.cmd_array,"pwd")==0){
        process_pwd_call(list);
    }
    else if(strcmp(*list.cmd_array,"exit")==0){
        process_exit_call(list);
    }
    else{
        process_external_command_redirection(list);
    }
    reset_redi();
}

void exec_command_pipeline(cmd_struct list,int pid){
    if(pid==0){
        defaultSignals();
        if(strcmp(*list.cmd_array,"cd")==0){
            process_cd_call(list);
        }
        else if(strcmp(*list.cmd_array,"pwd")==0){
            process_pwd_call(list);
        }
        else if(strcmp(*list.cmd_array,"exit")==0){
            process_exit_call(list);
        }
        else{
            process_external_command_pipeline(list,pid);
        }
        exit(errorCode);
    }
}

/**
 * Compare a string to multiple redirection signs (<,>,>|,>>,2>,2>|,2>>)
 * @param str the string to compare to
 * @return boolean: 1=true 0=false
 */
int strcmp_redirections(char* str) {
    char *list[] = {"<", ">", ">|", ">>", "2>", "2>|", "2>>"};

    for (int i = 0; i < 7; ++i) {
        if (strcmp(list[i], str) == 0) {
            return 1;
        }
    }
    return 0;
}

char* in_redir(cmd_struct cmd){
    for(int i=0;i<cmd.taille_array;i++){
        if(strcmp(*(cmd.cmd_array+i),"<")==0){
            if(i+1==cmd.taille_array) perror_exit("input redirection");
            return *(cmd.cmd_array+i+1);
        }
    }
    return "stdin";
}

char** out_redir(cmd_struct cmd){
    char** res= malloc(sizeof(char*)*2);
    for(int i=0;i<cmd.taille_array;i++){
        if((strcmp(*(cmd.cmd_array+i),">")==0 || strcmp(*(cmd.cmd_array+i),">|")==0 || strcmp(*(cmd.cmd_array+i),">>")==0)){
            if(i+1==cmd.taille_array) perror_exit("out redirection");
            //todo malloc avant cpy
            strcpy(*res,*(cmd.cmd_array+i));
            strcpy(*(res+1),*(cmd.cmd_array+i+1));
            return res;
        }
    }
    *res="stdout";
    return res;
}

char** err_redir(cmd_struct cmd){
    char** res=malloc(sizeof(char*)*2);
    for(int i=0;i<cmd.taille_array;i++){
        if((strcmp(*(cmd.cmd_array+i),"2>")==0 || strcmp(*(cmd.cmd_array+i),"2>|")==0 || strcmp(*(cmd.cmd_array+i),"2>>")==0)){
            if(i+1==cmd.taille_array) perror_exit("stderr redirection");
            // todo malloc avant cpy
            strcpy(*res,*(cmd.cmd_array+i));
            strcpy(*(res+1),*(cmd.cmd_array+i+1));
            return res;
        }
    }
    *res="stderr";
    return res;
}

static size_t pipes_quantity=0;

/**
 * Creates a cmds_struct with the commands separated by the redirection symbols.
 * exemple : [0] "ls -l | wc -l" -> [0] "ls -l" [0] "|" [0] "wc -l"
 * avec chaque correspondant à [0] un tableau différent
 * @param cmd the struct with an array of strings of the command line
 * @return cmds_struct with the commands separated
 */
cmds_struct separate_redirections(cmd_struct cmd){
    cmds_struct res;
    res.taille_array=0;
    cmd_struct* cmds_array=malloc(sizeof(cmd_struct)*MAX_ARGS_NUMBER);

    int string_count=0;
    int string_count_cmds=0;
    // index of the last copied string
    int size_tmp=0;
    while(string_count<cmd.taille_array){
        // when the current string is a redirection symbol
        if(strcmp(*(cmd.cmd_array+string_count),"|")==0){
            pipes_quantity++;
            // copy all strings before the symbol in one array
            (cmds_array+res.taille_array)->cmd_array=malloc(sizeof(char*)*string_count_cmds+1);
            testMalloc((cmds_array+res.taille_array)->cmd_array);
            memcpy((cmds_array+res.taille_array)->cmd_array,(cmd.cmd_array+size_tmp),sizeof(char*)*(string_count_cmds+1));
            size_tmp+=string_count_cmds;
            (cmds_array+res.taille_array)->taille_array=string_count_cmds;

            res.taille_array++;
            /*
            // copy the symbol in the next array
            (cmds_array+res.taille_array+1)->cmd_array=malloc(sizeof(char*)*4);
            testMalloc((cmds_array+res.taille_array+1)->cmd_array);
            memcpy((cmds_array+res.taille_array+1)->cmd_array,(cmd.cmd_array+size_tmp),sizeof(char*)*(4));
            (cmds_array+res.taille_array+1)->taille_array=1;
            size_tmp+=1;

            res.taille_array+=2;
            */
            size_tmp+=1;
            string_count_cmds=0;
        }
        else{
            string_count_cmds++;
        }
        string_count++;
    }

    // copy either the last part of the string or the string without redirections
    (cmds_array+res.taille_array)->cmd_array=malloc(sizeof(char*)*(string_count_cmds+1));
    testMalloc((cmds_array+res.taille_array)->cmd_array);
    memcpy((cmds_array+res.taille_array)->cmd_array,(cmd.cmd_array+size_tmp),sizeof(char*)*(string_count_cmds+1));
    (cmds_array+res.taille_array)->taille_array=string_count_cmds;
    res.taille_array+=1;
    cmds_array=realloc(cmds_array,sizeof(cmd_struct)*res.taille_array);
    res.cmds_array=cmds_array;
    return res;
}

static char* input_redir;
static char** output_redir;
static char** error_redir;

void handle_pipe(cmds_struct cmds){
    size_t num_commands=cmds.taille_array;
    input_redir=in_redir(*cmds.cmds_array);
    output_redir=out_redir(*(cmds.cmds_array+num_commands-1));
    int output_fd;

    // create a pipe for each pair of adjacent commands
    size_t num_pipes=num_commands-1;
    int pipes[num_pipes][2];
    for (int i=0; i<num_pipes; i++) {
        if (pipe(pipes[i])<0) {
            perror_exit("pipe");
        }
    }

    // create a child process for each command
    pid_t child_pids[num_commands];
    for (int i=0; i<num_commands; i++) {
        child_pids[i]=fork();
        if (child_pids[i]<0) {
            perror_exit("fork");
        }
        if(child_pids[i]==0) break;
    }

    // set up the pipeline and redirections in the child processes
    for (int i=0; i<num_commands; i++) {
        if (child_pids[i]==0) {
            // redirect standard input
            if (i==0){
                if(strcmp(input_redir,"stdin")==0) {
                    // redirect standard input from a file
                    int input_fd=open(input_redir,O_RDONLY);
                    if (input_fd<0){
                        perror_exit("open");
                    }
                    dup2(input_fd,STDIN_FILENO);
                    close(input_fd);
                }
            }
            else if (i>0) {
                dup2(pipes[i-1][0],STDIN_FILENO);
            }
            if (i<num_commands-1) {
                dup2(pipes[i][1],STDOUT_FILENO);
            }
            // redirect standard output
            else if(strcmp(*output_redir,"stdout")==0){
                // redirect standard output to a file
                if(strcmp(*output_redir,">") == 0){
                  output_fd = open(*(output_redir+1), O_WRONLY | O_CREAT | O_EXCL);
                }
                else if(strcmp(*output_redir,">>") == 0){
                  output_fd = open(*(output_redir+1), O_WRONLY | O_CREAT | O_APPEND);
                }
                else if(strcmp(*output_redir,">|") == 0){
                  output_fd = open(*(output_redir+1), O_WRONLY | O_CREAT | O_TRUNC);
                }

                if (output_fd<0) {
                    perror_exit("open");
                }
                dup2(output_fd,STDOUT_FILENO);
                close(output_fd);
            }

            //redirect error output
            error_redir = err_redir((*(cmds.cmds_array+i)));
            if(strcmp(*error_redir,"sdterr")==0){
              // redirect error output to a file
              if(strcmp(*output_redir,"2>") == 0){
                output_fd = open(*(error_redir+1), O_WRONLY | O_CREAT | O_EXCL);
              }
              else if(strcmp(*output_redir,"2>>") == 0){
                output_fd = open(*(error_redir+1), O_WRONLY | O_CREAT | O_APPEND);
              }
              else if(strcmp(*output_redir,"2>|") == 0){
                output_fd = open(*(error_redir+1), O_WRONLY | O_CREAT | O_TRUNC);
              }

              if (output_fd<0) {
                  perror_exit("open");
              }
              dup2(output_fd,STDERR_FILENO);
              close(output_fd);
            }

            // close all unnecessary pipe ends
            for (int j=0; j<num_pipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // execute the command
            //dprintf(STDERR_FILENO,"%s\n",*((cmds.cmds_array+i)->cmd_array));
            exec_command_pipeline(*(cmds.cmds_array+i),child_pids[i]);
            perror_exit("execvp");
        }
    }

    // close all unnecessary pipe ends in the parent process
    for (int i=0; i<num_pipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // wait for all child processes to complete
    for (int i=0; i<num_commands; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
        errorCode = WEXITSTATUS(status);
        if(strcmp(*((cmds.cmds_array+i)->cmd_array),"exit")==0){
            exit(errorCode);
        }
        if(errorCode==1) break;
        if(WIFSIGNALED(status)) errorCode = -1;
    }
}




void handle_redirection(cmd_struct cmd){
    char** line=cmd.cmd_array;
    int in_flags=O_RDONLY;
    int out_flags=O_WRONLY;
    int err_flags=O_WRONLY;

    int i=0;
    while(i<cmd.taille_array){
        // input redirection
        if(strcmp(*(line+i),"<")==0){
            i++;
            int in_fd=open(*(line+i),in_flags, S_IRUSR);
            if(in_fd<0) perror_exit("open");
            saved[0]=dup(0);
            if(dup2(in_fd,STDIN_FILENO)<0) perror_exit("dup2");
            if(close(in_fd)<0) perror_exit("close");
        }
        // output redirection
        else if(strcmp(*(line+i),">")==0 || strcmp(*(line+i),">|")==0 || strcmp(*(line+i),">>")==0){
            if(strcmp(*(line+i),">")==0){
                //out_flags|=O_CREAT | O_EXCL;
                out_flags=O_RDWR | O_CREAT | O_EXCL;
            }
            else if(strcmp(*(line+i),">>")==0){
                //out_flags|=O_CREAT | O_APPEND;
                out_flags=O_RDWR | O_CREAT | O_APPEND;
            }
            else if(strcmp(*(line+i),">|")==0){
                //out_flags|=O_TRUNC;
                out_flags=O_RDWR | O_TRUNC;
            }
            i++;
            int out_fd=open(*(line+i),out_flags, S_IRUSR | S_IWUSR);
            if(out_fd<0) perror_exit("open");
            saved[1]=dup(1);
            if(dup2(out_fd,STDOUT_FILENO)<0) perror_exit("dup2");
            if(close(out_fd)<0) perror_exit("close");
        }
        // error redirection
        else if(strcmp(*(line+i),"2>")==0 || strcmp(*(line+i),"2>|")==0 || strcmp(*(line+i),"2>>")==0){
            if(strcmp(*(line+i),"2>")==0){
                //err_flags|=O_CREAT | O_EXCL;
                err_flags=O_RDWR | O_CREAT | O_EXCL;
            }
            else if(strcmp(*(line+i),"2>>")==0){
                err_flags=O_CREAT | O_RDWR | O_APPEND;
                //err_flags|=O_CREAT | O_EXCL;
            }
            else if(strcmp(*(line+i),"2>|")==0){
                //err_flags|=O_TRUNC;
                err_flags=O_RDWR | O_TRUNC;
            }
            i++;
            int err_fd=open(*(line+i),err_flags,0644);
            if(err_fd<0) perror_exit("open");
            saved[2]=dup(2);
            if(dup2(err_fd,STDERR_FILENO)<0) perror_exit("dup2");
            if(close(err_fd)<0) perror_exit("close");
        }
        i++;
    }
}

/***
 * Interprets the commands to call the corresponding functions.
 * @param liste struct for the command
 */
void interpreter_new(cmd_struct list) {
    cmds_struct separated_list=separate_redirections(list);
    if(separated_list.taille_array>1){
        handle_pipe(separated_list);
    }
    else{
        handle_redirection(*separated_list.cmds_array);
        exec_command_redirection(*separated_list.cmds_array);
    }
}

/*
void interpreter(cmd_struct list) {
    if (strcmp(*list.cmd_array, "cd") == 0) {
        process_cd_call(list);
    }
    else if (strcmp(*list.cmd_array, "pwd") == 0) {
        process_pwd_call(list);
    }
    else if (strcmp(*list.cmd_array, "exit") == 0) {
        process_exit_call(list);
    }
    else {
        process_external_command(list,0);
    }
}
*/


void joker_solo_asterisk(cmd_struct liste){
    //printf("joker_solo_asterisk \n");
    //on commence par ajouter la commande dans le tableau args
    char ** args = malloc(sizeof(char *));
    *(args) = NULL;
    // on combine args avec le nouveau char ** représentant les chaines obtenues en remplaçant * dans un argument de la liste.
    for(int i = 0; i < liste.taille_array; i++){
        // la fonction combine_char_array renvoie un nouveau pointeur char ** (malloc à l'intérieur)
        // la fonction replaceAsterisk renvoie un char ** (malloc à l'intérieur).
        char * suppressed_slash = supprimer_occurences_slash(*(liste.cmd_array+i));
        char** replace;
        // si suppressed_slash commence par **/ on appelle replaceDoubleAsterisk
        if(strlen(suppressed_slash) > 2 && suppressed_slash[0] == '*' && suppressed_slash[1] == '*' && suppressed_slash[2] == '/'){
            replace = replaceDoubleAsterisk(suppressed_slash);
        }else{ // sinon on utilise replaceAsterisk
            replace = replaceAsterisk(suppressed_slash);
        }

        //si n'a aucun string, alors donner liste.cmd_array[i]
        if(replace[0] == NULL){
            freeArray(replace);
            replace = malloc(sizeof(char *) * 2);
            replace[0] = malloc(sizeof(char) * (strlen(suppressed_slash) + 1));
            strcpy(replace[0],suppressed_slash);
            replace[1] = NULL;
        }
        char** tmp = copyStringArray(args);
        freeArray(args);
        args = combine_char_array(tmp,replace);
        freeArray(replace);
        freeArray(tmp);
        free(suppressed_slash);
    }
    cmd_struct new_liste;
    size_t tailleArray = 0;
    while(args[tailleArray] != NULL) tailleArray ++;
    new_liste.taille_array = tailleArray;
    new_liste.cmd_array = args;
    interpreter_new(new_liste);
    freeCmdArray(new_liste);
}

void joker_duo_asterisk(){

}


/***
 * Turns a line into a command structure.
 * @param ligne line from the prompt
 * @return struct cmd_struct
 */
cmd_struct lexer(char* ligne){
    char** cmd_array=malloc(sizeof(char*));
    testMalloc(cmd_array);
    char* token;
    size_t* taille_array_init= malloc(sizeof(size_t));
    testMalloc(taille_array_init);
    size_t taille_token;
    size_t taille_array=0;
    *taille_array_init=1;

    // take each string separated by a space and copy it into a list of strings
    token=strtok(ligne," ");
    do{
        taille_token=strlen(token);
        //todo test taille token
        if(taille_token>=MAX_ARGS_NUMBER){
            perror("MAX_ARGS_STRLEN REACHED");
            exit(1);
        }

        cmd_array=checkArraySize(cmd_array,taille_array,taille_array_init);
        *(cmd_array+taille_array)=malloc(sizeof(char)*(taille_token+1));
        *(cmd_array+taille_array)=memcpy(*(cmd_array+taille_array),token,taille_token+1);
        taille_array++;
    }
    while((token = strtok(NULL, " ")));


    free(token);
    free(taille_array_init);
    cmd_struct cmdStruct = {.cmd_array=cmd_array, .taille_array=taille_array};
    return cmdStruct;
}
