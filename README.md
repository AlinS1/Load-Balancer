**Similea Alin-Andrei**
**314CA**

## Load Balancer - SDA 2

### Implementation Description:

* Firstly, the defined structs are:
1. "load_balancer", which contains the hash ring (an array of pointers to
labels) and the number of servers that exist ("nr_servers").
2. "label", that contains the server_memory, the id of the server, "nr_label"
which is given by the formula given (replica_id * 10^5  + server_id) and its
hash. The thing important to mention is that even though there will be 3
different labels in the hash ring, all three of them will have the same pointer
to the corresponding server.

* For loader_add_server, we first deal with the labels and how to put them in
the hash ring. After we create the labels, we look for the position where the
label should be put considering that the labels' hashes need to be in ascending
order. Then, in order to make room for the new label, we move the labels
starting from that position one index to the left.
After the labels are put, we handle the objects. Firstly, we iterate through
the hash ring. When we reach a label that has the id of the server that was
just added, we move objects from the "next" server to the current one depending
on the respective case.

* For loader_remove_server, we first handle the objects.
A noteworthy special case is when both the first and last labels need to be
removed. This means that we will transfer the elements from these labels to the
second label in the hash ring. In order to memorize when this case applies, we
use an auxiliary variable "moved_first_n_last" which will be 1 if applicable.
After handling the objects, we need to free the memory of the labels that have
been removed. Because all 3 labels have a pointer to the same server, we will
need to free the memory of the server only ONCE. We do this by freeing the
memory of the server only for the original label (the label that has its number
equal to the server's id). Then, we free the label elements.
In order to reallocate the memory of the array, we need to get rid of the NULL
labels that we just freed. We do this by shifting the elements from the end of
the array towards the start.

* For loader_store and loader_retrieve, we need to search where would the
object's hash be in the hash ring and then work with the next label from that
position.

* For free_load_balancer, we iterate through the hash ring array. Because all 3
labels have a pointer to the same server, we will need to free the memory of
the server only once. We do this by freeing the memory of the server only for
the original label (the label that has its number equal to the server's id).
Then, we free the label elements from the array. After we free all elements
from the hash ring, we free the array and the load balancer.