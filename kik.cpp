#include <iostream>
#include <string.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <vector>

using namespace std;

#define klucz 2137

int* adres;

int pamiec;
int semafory;
int wynik = -9;

void board(int player){
	cout << endl << endl;
	int i, j;
    string litery = "ABC";
    player == 1 ? cout << "Twój znak 'O'" << endl : cout << "Twój znak 'X'" << endl;
	cout << "  1 2 3" << endl;
	for(i = 0; i < 3; i++){
        cout << litery[i];
		for(j = 0; j < 3; j++){
			cout << "|";
			int pole = *(adres + i * 3 + j);
			switch(pole){
				case -1:
					cout << "X";
					break;
				case 1:
					cout << "O";
					break;
				default:
					cout << " ";
					break;
			}
		}
		cout << "|" << endl;
	}
}

int jestWynik(){
    vector <int> arr(8);
	int i, j, remis = 0;
	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 3; j++)
		{
			int pole = *(adres + i * 3 + j);
			arr[0 + i] += pole;
			arr[3 + j] += pole;
			if(i == j)
				arr[6] += pole;
			else if(2-i == j)
				arr[7] += pole;
			if(pole != 0)
				remis++;
		}
	}

    if(remis == 9)
	    return 0;

	for(i = 0; i < 8; i++)
	{
		if(arr[i] == 3)
		{
			return 1;
		}
		else if(arr[i] == -3)
		{
			return -1;
		}
	}

	return -9;
}

void koniec(int player, int res){
	if(player != 0)
	{
		if(player == res)
		{
			cout << endl;
            cout << "***********************************\n";
            cout << "* Game over! The winner is YOU!   *\n";
            cout << "***********************************\n";
		}
		else if(res == 0)
		{
			cout << endl;
            cout << "***********************************\n";
            cout << "* Game over! It is a tie!         *\n";
            cout << "***********************************\n";
		}else
		{
			cout << endl;
            cout << "***********************************\n";
            cout << "* Game over! The winner is not YOU!*\n";
            cout << "***********************************\n";
		}
	}
	semctl(semafory, 0, IPC_RMID, 0);
	shmdt(adres);
	shmctl(pamiec, IPC_RMID, 0);
	exit(0);
}


void wyjscie(int niepotrzebne) {
	koniec(0, 0);
}


void zapisz(int player){
	board(player);
	cout << "\nWybierz pole (jak w statkach): " << endl;
	char x;
	int y, dalej = 1;
	do{
		scanf("%c%d", &x, &y);
        if(x < 65 || x > 67 || y < 1 || y > 3)
		{
            if(x >= 97 && x <= 99) {
                cout << "Spróbuj użyc wielkich liter" << endl;
            }else{
                cout << "Proszę celować w planszę, żeby mieć szansę na wygraną" << endl;
            }
		}
		else
		{
			int ix = x - 65;
            --y;
			int pole = *(adres + ix * 3 + y);
			if(pole != -1 && pole != 1)
			{
				*(adres + ix * 3 + y) = player;
				dalej = 0;
			}
			else
			{
				cout << "Proszę celować w niezajęte pola" << endl;
			}
		}
	}while(dalej);
	board(player);
}


int main(){
	
	cout << "\t\tT I C K -- T A C -- T O E -- G A M E\t\t" << endl;
    int errnum, player;
	signal(SIGINT, wyjscie);

	struct sembuf wait_0 = {0, -1, 0}, signal_1 = {1, 1, 0};
	struct sembuf wait_1 = {1, -1, 0}, signal_0 = {0, 1, 0};
    struct sembuf *p1, *p2;

	pamiec = shmget(klucz, 256, 0777 | IPC_CREAT);
    if (pamiec < 0){
        errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error opening file[%s]: %s\n", "pamiec", strerror( errnum ));
        cout << "Nie udało się wysłać danych do serwera" << endl;
        exit(1);
    }
	adres = (int *)shmat(pamiec, 0, 0);
    if ((void*)-1 == adres){
        errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error opening file[%s]: %s\n", "adres", strerror( errnum ));
        cout << "Nie udało się wysłać danych do serwera" << endl;
        exit(1);
    }


	if((semafory = semget(klucz, 2, 0777 | IPC_CREAT | IPC_EXCL)) != -1)
	{
		cout << "Zaczynasz jako pierwszy, czyli posługujesz się 'X'" << endl;
		player = -1;

		p1 = &wait_0;
		p2 = &signal_1;

		semctl(semafory, 0, SETVAL, 1);
		semctl(semafory, 1, SETVAL, 0);
	}
	else
	{
		semafory = semget(klucz, 2, 0777 | IPC_CREAT);
		
		cout << "Posługujesz się 'O', czyli jesteś drugi. Poczekaj na swoją kolej" << endl;
		player = 1;
		
		p1 = &wait_1;
		p2 = &signal_0;
	}

	while(true){
		semop(semafory, p1, 1);
		wynik = jestWynik();
		if(wynik != -9)
		{
			board(player);
			koniec(player, wynik);
			break;
		}
		zapisz(player);
		wynik = jestWynik();
		if(wynik != -9)
		{
			board(player);
			koniec(player, wynik);
			break;
		}
		
		semop(semafory, p2, 1);
	}
	return 0;
}
