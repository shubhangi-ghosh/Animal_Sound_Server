#include<stdio.h> 
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<iostream>
#include<fstream>  
#include<unordered_map>
#include<algorithm>			//For transform 
using namespace std; 

#define SERV_TCP_PORT 5035
#define MAXLINE 4096
#define FD_SETSIZE 15	//max fds/ terminals

/*****************************************************************************************************************
									LOGIN DATABASE MANIPULATION FUNCTIONS
*****************************************************************************************************************/

void store(string username, string password) 
{
	FILE *file = fopen("login", "a");
	char msg[128];

	if(!file)
    {
      strcpy(msg,"\nFatal error: Couldn't save changes made.\n");
      write(1,msg, strlen(msg));
      exit(0);
    }

    //for(auto iter=map.begin(); iter!=map.end(); iter++)
    //{
    fprintf(file, "%s=%s\n", username.c_str(), password.c_str());
    //}

    strcpy(msg,"\nChanges saved successfully.\n");
    write(1,msg, strlen(msg));
    fclose(file);
}


void load(unordered_map<string,string> &map) {

	FILE *file = fopen("login", "r+");		//file to store login data opened
	char msg[128];

	//if error in opening file
    if(!file){
        strcpy(msg, "\nFatal error: Couldn't load required files. Try running the server again.\n");
        write(1,msg, strlen(msg));	//1:stdout/terminal  
        exit(0);	//server closed and gives zero in client side
    }

    char *buf = 0;	//read from file and store here
    size_t buflen = 0;	


    while(getline(&buf, &buflen, file) > 0) 
    {
        char *nl = strchr(buf, '\n'); 
        if (nl == NULL)
            continue;
        *nl = 0;

        char *sep = strchr(buf, '=');
        if (sep == NULL)
            continue;
        *sep = 0;
        sep++;

        string s1 = buf;
        string s2 = sep;

        map[s1] = s2;

    }

    if (buf)
        free(buf);

    strcpy(msg,"\nLoaded login database successfully..");
    write(1,msg, strlen(msg));

    fclose(file);
}

/*****************************************************************************************************************
										ANIMAL SOUND CLASS
*****************************************************************************************************************/


class Animal{

	static unordered_map<string, string> sounds;

	public:

		static unordered_map<string,string> init()
		{
			
			unordered_map<string, string> sounds;

			sounds["dog"]   = "woof";
			sounds["cat"]   = "meow";
			sounds["bear"]  = "growl";
			sounds["horse"] = "neigh";
			sounds["chick"] = "cluck";

			return sounds;
		}

		string getAllSounds() 			//query operation
		{

			string res = "\nI know the sound of:\n";

			for(auto animal: sounds){

				res += "\n  =>" + animal.first;
			}

			res += "\n\n****************************************************\n";

			return res;

		}

		void storeSound(string animal, string sound)		//store operation
		{
			sounds[animal] = sound;
		}

		string getSound(string animal){						//to get sound of an animal

			if(sounds.find(animal) == sounds.end())
				return "";
			else
				return "A " + animal + " says " + sounds[animal] + "\n";

		}

};





unordered_map<string,string> Animal::sounds = Animal::init();




int main()
{

/*****************************************************************************************************************
										LOGIN DATABASE INTIALIZATION
*****************************************************************************************************************/

	unordered_map<string, string> login;
	// login["Shruti"]    = "hello";
	// login["Ritika"]    = "ritika";
	// login["Sunidhi"]   = "dahio";
	// login["Sanyam"]    = "sanyam";
	// login["Shubhangi"] = "vikas";
	load(login);
	


/*****************************************************************************************************************
										DATA VARIABLE INTIALIZATION
*****************************************************************************************************************/

	int                 		i, maxi, maxFd, listenFd, connFd, sockFd;
    int                 		nready, client[FD_SETSIZE];
    int                 		n;
    bool 						changed = 0;				//for login database
    fd_set              		rset, allset;
    char                		recv[MAXLINE], dataExpecting[FD_SETSIZE];
    socklen_t           		clilen;
    struct sockaddr_in  		cliaddr, serv;
	string 						username[FD_SETSIZE];
	unordered_map<int,string>	arguments;					 
	Animal 						animal;

/*****************************************************************************************************************
										SOCKET AND SERVER ADDRESS INITIALIZATION
*****************************************************************************************************************/

	listenFd = socket(AF_INET, SOCK_STREAM, 0); //ipv4 socket that uses tcp
	
	bzero(&serv,sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = htonl(INADDR_ANY);//load wildcard ip adress in network byte order
	serv.sin_port = htons(SERV_TCP_PORT);//load port 


/*****************************************************************************************************************
							BIND SOCKET TO SERVER ADDRESS AND BRING IT TO LISTENING STATE
*****************************************************************************************************************/
	bind(listenFd,(struct sockaddr *) &serv, sizeof(serv));
	
	write(1,"\nServer in listening state ...\n",30);
	listen(listenFd,10);



/*****************************************************************************************************************
										SELECT VARIABLE INTIALIZATION
*****************************************************************************************************************/

	maxFd = listenFd;
	maxi = -1;

	
	for(i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;
		dataExpecting[i] = 'c';					//c for choice
	}
	
	FD_ZERO(&allset); //clear bits in allset
	FD_SET(listenFd,&allset);	//turn on bit for listenfd in allset
	
	for(;;){
	
		rset = allset;

		bzero(&recv,sizeof(recv));
		nready = select(maxFd + 1, &rset, NULL, NULL, NULL); //return number of ready descriptors
		


/*****************************************************************************************************************
								LISTENFD BECOMES READABLE i.e. CONNECT IS CALLED
*****************************************************************************************************************/
		if(FD_ISSET(listenFd, &rset)) //checks if listenfd is set in rset?
		{
			socklen_t len = sizeof(cliaddr);

			connFd = accept(listenFd,(struct sockaddr *) &cliaddr , &len); //accept connection on lisendfd coming from cliaddr

			if(connFd == -1) {
				printf("\nNot connected!\n");
				break; 
			}
			
			char str[100];

			sprintf(recv,"\nNew client: %s, port %d, connFd %d\n",
                 	inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                    ntohs(cliaddr.sin_port), connFd); 

			write(1,recv,strlen(recv));

			for(i = 0; i < FD_SETSIZE; i++)
				if(client[i] < 0) {
					client[i] = connFd;
					break;
				}
				
			if(i == FD_SETSIZE) {
				printf("\nWe are facing huge number of requests. Please try again later.\n");
				return 1;
			}
			
			char reply[MAXLINE] = "\nConnected to ANIMAL SOUND.\nTo continue choose: \n 1. REGISTER\n 2. LOGIN.\n ";

			write(connFd, reply, strlen(reply));

			FD_SET(connFd,&allset);
			
			if(connFd > maxFd)
				maxFd = connFd;
			
			if(i > maxi)
				maxi = i;
			
			if(--nready <= 0) 
				continue; 
				
		}


/*****************************************************************************************************************
								CLIENT BECOMES READABLE i.e. CONNECTED SOCKET 
*****************************************************************************************************************/
	
		for(i = 0; i <= maxi; i++) {
			if(client[i] < 0)
				continue;
			
			if(FD_ISSET(client[i],&rset)) 
			{
				sockFd = client[i];
				n = read(sockFd, recv, MAXLINE);
				
				/********************************************************************************************
										IF CLIENTS CLOSES THE CONNECTION
				*********************************************************************************************/

				if( n == 0)
				{
					char reply[MAXLINE];
					sprintf(reply, "Ending connection\n");
					
					write(sockFd, reply, strlen(reply));
					close(sockFd);
					FD_CLR(sockFd, &allset);
					
					client[i] = -1;
					username[i] = "";
					dataExpecting[i] = 'c';
				
				}
				
				else 

				/********************************************************************************************
											IF CLIENTS SENDS SOME REQUEST
				*********************************************************************************************/

				{	
					/****************************************************************************************
					*					DATAEXPECTING[I]		  | 		MEANING							*
					**********************************************|******************************************
					*							c 				  | 		 choice                         *
					*							r 				  | 		register username               *
					*                           P                 |         register password               *
					*							u                 |         login username                  *
					*                           p                 |         login password                  *
					*                           d                 |         request                         *
					*****************************************************************************************/




					/*****************************************************************************************
											SERVER EXPECTING CHOICE EITHER REGISTER OR LOGIN
					*****************************************************************************************/

					if(dataExpecting[i] == 'c') 
					{
						string choice = string(recv);
						choice = choice.substr(0,choice.size()-1);
						char reply[MAXLINE];


						/************************************************************************************
											CLIENT CHOOSES REGISTER
						*************************************************************************************/

						if(choice == "1")
						{
							dataExpecting[i] = 'r';
							strcpy(reply,"\n************************************************\n                   REGISTER\n************************************************\n Enter username: ");
						}
						else 
						{
							dataExpecting[i] = 'u';
							strcpy(reply, "\n************************************************\n                    LOGIN\n************************************************\n Enter username: ");
						}

						write(sockFd, reply, strlen(reply));
					}

					/*****************************************************************************************
											SERVER EXPECTING USERNAME FOR REGISTERATION
					*****************************************************************************************/

					else if(dataExpecting[i] == 'r')
					{
						string uname = string(recv);
						uname = uname.substr(0,uname.size()-1);
						char reply[MAXLINE];

						if(login.find(uname)!=login.end()) 
						{
							strcpy(reply,"\nUsername already exist. Try logging in.\nMAIN MENU: \n 1. REGISTER\n 2. LOGIN.\n  \n");
							dataExpecting[i] = 'c';
						}
						else 
						{
							strcpy(reply,"\nEnter password: ");
							dataExpecting[i] = 'P';
							username[i] = uname;
						}

						write(sockFd, reply, strlen(reply));		
					}

					/*****************************************************************************************
											SERVER EXPECTING PASSWORD FOR REGISTERATION
					*****************************************************************************************/

					else if(dataExpecting[i] == 'P')
					{
						string password = string(recv);
						password = password.substr(0,password.size()-1);
						char reply[MAXLINE];

						if(password.size() < 8) 
						{
							strcpy(reply,"\nPassword must be more than or equal to 8. Try again.\nEnter password: ");
							dataExpecting[i] = 'P';
						}
						else
						{
							login[username[i]] = password;
							strcpy(reply,"\nRegistered Successfully. Now login for further processing. \n************************************************\n                    LOGIN\n************************************************\n Enter username: ");
							dataExpecting[i] = 'u';
							store(username[i],password);
						}

						write(sockFd, reply, strlen(reply));

					}

					/*****************************************************************************************
											SERVER EXPECTING USERNAME FOR LOGIN
					*****************************************************************************************/

					else if(dataExpecting[i] == 'u')
					{	

						string uname = string(recv);
						uname = uname.substr(0,uname.size()-1);
						char reply[MAXLINE];

						if(login.find(uname)==login.end()) {
							strcpy(reply,"\nUsername does not exist. Register yourself first.\nMAIN MENU: \n 1. REGISTER\n 2. LOGIN.\n ");						
							dataExpecting[i] = 'c';
						}

						else 
						{
							dataExpecting[i] = 'p';
							username[i] = uname;
							strcpy(reply,"\n Enter password: ");
						}			

						write(sockFd,reply,strlen(reply));	
					}
					
					/*****************************************************************************************
											SERVER EXPECTING PASSWORD FOR LOGIN
					*****************************************************************************************/

					else if(dataExpecting[i] == 'p')
					{
						
						string password = string(recv);
						password = password.substr(0,password.size()-1);
						char reply[MAXLINE];

						if(login[username[i]] == password) 
						{
							dataExpecting[i] = 'd';
							strcpy(reply,"\nSuccessfully logged in.\n\n************************************************\n              WELCOME TO SOUND SERVER\n************************************************\n\nTo check your connection anytime enter \'SOUND\' and if you get \'SOUND OK\' msg in return then your connection is fine.\n");
							strcat(reply,"\n\n Available options: \n => QUERY : To check which animal I know");
							strcat(reply,"\n => STORE : To store (animal, sound) pair");
							strcat(reply,"\n => BYE : To end your current session");
							strcat(reply,"\n => END : To close the server completely\n");

						}
						else 
						{ 
							dataExpecting[i] = 'u';
							strcpy(reply,"\nUsername and password does not match. Try again.\nEnter username:");
						}

						write(sockFd,reply,strlen(reply));							
					}	
					
					/*****************************************************************************************
											SERVER EXPECTING SOME REQUEST
					*****************************************************************************************/

					else if (dataExpecting[i] == 'd')
					{
						string request = string(recv);
						char reply[MAXLINE];

						request = request.substr(0,request.size()-1);
						transform(request.begin(), request.end(), request.begin(), ::tolower); 	

						if(request == "sound")
							strcpy(reply, "SOUND OK\n\n");

						else if(request == "store") {
							dataExpecting[i] = '2';			//Number of arguments required
							strcpy(reply,"\n Animal:");
						}

						else if(request == "query") 
							strcpy(reply, animal.getAllSounds().c_str()); 

						else if(request == "bye") {
							strcpy(reply,"\nAdios amigo! See you soon!\n");
							write(sockFd, reply, strlen(reply));
							FD_CLR(sockFd, &allset);
							close(sockFd);
							client[i] = -1;
							dataExpecting[i] ='u';
						}

						else if(request == "end") {
							sprintf(reply, "\nServer ended by client %d", client[i]);
							write(1,reply,strlen(reply));

							sprintf(reply, "Ending server on your request\n");
							write(sockFd, reply, strlen(reply));
							exit(0);
						}
						else
						{
							string sound = animal.getSound(request);
							if(sound == "")
								sprintf(reply, "I don't know about %s :( \nYou can teach me by using \'STORE\' command.\n",request.c_str());
							else
								strcpy(reply, sound.c_str()); 

						} 

						
						write(sockFd, reply, strlen(reply));

					}

					/*****************************************************************************************
											SERVER EXPECTING SOME ARGUMENTS
					*****************************************************************************************/

					else if(dataExpecting[i] == '2')			//Arg1
					{
						string arg1 = string(recv);
						arg1 = arg1.substr(0,arg1.size()-1);

						arguments[client[i]] = arg1;
						dataExpecting[i] = '1';

						char reply[MAXLINE] = "\nSound:";
						write(sockFd, reply, strlen(reply));
					}

					else if (dataExpecting[i] == '1')			//Arg2
					{
						string arg2 = string(recv);
						arg2 = arg2.substr(0,arg2.size()-1);

						animal.storeSound(arguments[client[i]],arg2);

						char reply[MAXLINE];
						sprintf(reply,"Yippie! I now know about a %s too!\n", arguments[client[i]].c_str());
						dataExpecting[i] = 'd';

						write(sockFd, reply, strlen(reply));

					}

				}
				
				if (--nready <= 0)
                    break;	
			}
			
		}
		
	}	

}
