**Similea Alin-Andrei**
**314CA**

## Load Balancer - SDA 2

### Implementation Description:

* Firstly, the defined structs are:
1. "load_balancer", that contains the hash ring (an array of pointers to labels) and the number of servers that exist ("nr_servers").
2. "label", that contains the server_memory, the id of the server, "nr_label" which is given by the formula given (replica_id * 10^5  + server_id) and its hash. The thing important to mention is that, even though there will be 3 different labels in the hash ring, all three of them will have the same pointer to the corresponding server.







### Comentarii asupra temei:

* Crezi că ai fi putut realiza o implementare mai bună?
* Ce ai invățat din realizarea acestei teme?
* Alte comentarii