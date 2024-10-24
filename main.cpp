#include <iostream>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>
#include <random>

using namespace std;


int n = 20; // Nombre de places disponibles


mutex nbCustomerMut;
int nbCustomer = 0;
int nbBarbers = 3;

mutex registerMut;

counting_semaphore<4> sofaSem(4);

counting_semaphore<1> customerSem(0);
counting_semaphore<1> barberSem(0);

counting_semaphore<1> customerDone(0);
counting_semaphore<1> barberDone(0);

counting_semaphore<1> customerPaySem(0);
counting_semaphore<1> barberPaySem(0);
counting_semaphore<1> paymentDone(0);


mutex sofaMut;
vector<thread::id> vecClientsSofa(0);


mutex deboutMut;
vector<thread::id> vecClientDebout(0);



void balk(const string & nom)
{
    cout << nom << " walks away." << endl;
}

void getHairCut(const string & nom)
{
    cout << nom << " is getting a haircut." << endl;
}

void cutHair(const string & barberNom)
{
    cout << barberNom << " is cutting hair." << endl;
}

void pay(const string & nom)
{
    cout << nom << " pays." << endl;
}

void acceptPayment(const string & barberNom)
{
    cout << barberNom << " accepts the payment." << endl;
}

void customer(const string & nom)
{
    thread::id tId = this_thread::get_id();



    nbCustomerMut.lock();   //demande nbCustomer


    cout << nom << " enters the barbershop." << endl;
    if (nbCustomer == n)
    {
        nbCustomerMut.unlock();     //libere nbCustomer

        balk(nom);
        return;

    }
    //Debut list clients debouts ( C'EST PAS FINI )
    deboutMut.lock();


    vecClientDebout.push_back(tId);
    deboutMut.unlock();


    ++nbCustomer;

    nbCustomerMut.unlock();     //libere nbCustomer


    cout << "AHHHH"<<endl;
    cout << vecClientDebout.at(0)<<endl;
    cout << this_thread::get_id()<<endl;
    cout << "AHHHH"<<endl;
    while (true) {
        lock_guard<mutex> lock(deboutMut); // Protection du vecteur lors de la vérification
        if (vecClientDebout.front() == this_thread::get_id()) {
            break; // Si ce thread est le premier, on peut sortir de la boucle
        }
    }

    deboutMut.lock();
    vecClientDebout.erase(vecClientDebout.begin());
    sofaSem.acquire();
    vecClientsSofa.push_back(this_thread::get_id());
    deboutMut.unlock();




    cout << nom << " is waiting to sit on the sofa." << endl;

    cout << nom << " is sitting on the sofa, waiting for a barber." << endl;

    customerSem.release();
    barberSem.acquire();

    sofaMut.lock();
//    while (vecClientSofa.front() != this_thread::get_id()) {
//
//        this_thread::sleep_for(chrono::milliseconds(50));
//    }

    vecClientsSofa.erase(vecClientsSofa.begin());
    sofaSem.release(); //Libere une place sur le sofa
    sofaMut.unlock();


    getHairCut(nom);

    customerDone.release();
    barberDone.acquire();





    customerPaySem.release(); // Va a la caisse
    barberPaySem.acquire(); //Attend un barber

    pay(nom); //Le client paye

    paymentDone.acquire(); //Attend que le paiement soit validé

    //Le client s'en va

    nbCustomerMut.lock();   //demande nbCustomer





    --nbCustomer;           //décrémente nbCustomer

    nbCustomerMut.unlock(); //libere nbCustomer


}

bool endOfLoop = false;
unsigned int haricutGiven = 0;


void barber(const string & barberNom)
{

    // Le barber boucle pour prendre continuellement prendre en charge les clients
    while (true)
    {
        customerSem.acquire();
        barberSem.release();

        // Condition de sortie de la boucle
        if (endOfLoop) {
            cout << barberNom << " ended his shift."<<endl;
            break;
        }
        cutHair(barberNom);

        customerDone.acquire();
        barberDone.release();
        ++haricutGiven;

        barberPaySem.release();
        customerPaySem.acquire(); //Attend le client a la caisse
        registerMut.lock(); // Le barbeur réserve la caisse

        // le client paye

        acceptPayment(barberNom); // Le barbeur accepte le paiement

        registerMut.unlock(); // Le barbeur libere la caisse
        paymentDone.release(); // Le barbeur signale que le paiement est fait

        cout << barberNom <<" is ready" << endl;

    }

}






int main()
{
    unsigned customerWave = 200;

    vector<thread> vecThreadBarbers(nbBarbers);
    vector<thread> vecThreadCustomers(customerWave);



    //On lance tous les threads barber
    for (int i = 0; i < vecThreadBarbers.size(); ++i) {

        vecThreadBarbers.at(i) = thread(barber, "Barber " + to_string(i));
    }

    //On lance tous les threads client
    for (int i = 0; i < vecThreadCustomers.size(); ++i) {

        vecThreadCustomers.at(i) = thread(customer, "Client " + to_string(i));
    }

    //On join tous les threads client
    for (int i = 0; i < vecThreadCustomers.size(); ++i) {
        vecThreadCustomers.at(i).join();
    }

    // Il y a 2 boucles séparées pour être sur que lors des joins tous les barbers soient sortis de leur boucle.

    //Condition de sortie de la boucle
    endOfLoop = true;

    for (int i = 0; i < vecThreadBarbers.size(); ++i) {
        //Faux client pour que les barbers n'attendent pas indéfiniment
        customerSem.release();
    }

    //On join tous les threads barber
    for (int i = 0; i < vecThreadBarbers.size(); ++i) {

        vecThreadBarbers.at(i).join();
    }

    cout << "Haircut given : "<<haricutGiven << endl; // Nombre de clients pris en charge
    cout << "Customer that walked away : "<< customerWave - haricutGiven << endl; // Nombre de clients non pris en charge
    cout << "Haircut rate : "<< (double(haricutGiven) / double(customerWave))*100<< "%" << endl; //Pourcentage de clientq pris en charge






    return 0;
}