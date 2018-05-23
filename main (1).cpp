#include <iostream>
#include <cstdlib>
#include <string.h>
#include <cstring>
#include <math.h>
#include <fstream>
using namespace std;

//global variables
int sets =1, assoc = 1; //(rows and columns)
unsigned long long int blocksize = 0, cache_size = 0; //(taken from user)
int replace_policy = 0, write_policy = 0;
long long int lru_counter =0;

class cache
{
    public:
    unsigned long long int tag;           // incoming address tag value
    unsigned long long int r_count;       // read count
    unsigned long long int r_hit_count;   // read hit count
    unsigned long long int r_miss_count;  // read miss count
    unsigned long long int w_count;       // write count
    unsigned long long int w_hit_count;   // write hit count
    unsigned long long int w_miss_count;  // write miss count
    unsigned long long int wb_count;      // write back count
    unsigned long long int index;         // incoming address index value

    int **valid_bit; // allocate valid bit in a 2d array
    unsigned long long int **lru_count;  // allocate lru count in a 2d array
    unsigned long long int **lfu_count;  // allocate lfu count in a 2d array
    unsigned long long int **age; // allocate age in a 2d array
    int **dirty_bit; // allocate dirty bit in a 2d array
    int **tag_store; // allocate space for hex tag value in a 2d array

    //member functions -
    int read(unsigned long long int tag,unsigned long long int index);
    int write(unsigned long long int,unsigned long long int);
    string hex_char_to_bin(string);
    int bintodec(string, int);
    int update_counter(unsigned long long int,int,int); //(row,column,hit/miss)
    int evictblock();
    cache(int,int); //constructor function (rows and columns)

};

cache::cache(int sets,int assoc)
    {
    tag =0;
    r_count=0;
    r_hit_count=0;
    r_miss_count=0;
    w_count=0;
    w_hit_count=0;
    w_miss_count=0;
    wb_count=0;
    index=0;

    int i=0;
    valid_bit=new int *[sets]; //*[sets] = pointer to 1D array of 'sets'number of integers.(array created in every row)
    for(i=0;i<sets;i++)
        valid_bit[i]=new int [assoc]; // one validbit to every column(block) in that row

    lru_count=new unsigned long long int*[sets]; //lru count
    for(i=0;i<sets;i++)
        lru_count[i]=new unsigned long long int[assoc];

    lfu_count = new unsigned long long int*[sets]; //lfu count
    for(i=0;i<sets;i++)
        lfu_count[i]=new unsigned long long int[assoc];

    age = new unsigned long long int*[sets]; //age value
    for(i=0;i<sets;i++)
        age[i]=new unsigned long long int[assoc];

    dirty_bit = new int*[sets]; //dirty bit for each block
    for(i=0;i<sets;i++)
        dirty_bit[i]=new int[assoc];

    tag_store = new int*[sets];  //tagstore for saving tag value corresponding to every block in the set
    for(i=0;i<sets;i++)
        tag_store[i]=new int[assoc];

    //Initialization of values in each of those bits //assigning memory to valid_bit,dirty_bit,lfu_count,lru_count,tagstore
    for( i=0;i<sets;i++)
    {
        for(int j=0;j<assoc;j++)
        {
            valid_bit[i][j] = 0;
            dirty_bit[i][j] = 0;
            lru_count[i][j] = assoc-1-j; // make lru counter values 3,2,1,0 etc
            lfu_count[i][j] = 0;
            tag_store[i][j] = 0;
            age[i][j] = 0;
        }
    }
}
//this function changes hex string value in input file to binary string
string cache::hex_char_to_bin(string Hex)//input is some string
		{
			string sReturn = ""; //empty string
			for (int i = 2; i < Hex.length(); ++i) //hex.length()= length of input string in bytes
			{
				switch (Hex [i])//each character in that string
				{
					case '0': sReturn.append ("0000"); break; //make the empty string as 0000
					case '1': sReturn.append ("0001"); break;
					case '2': sReturn.append ("0010"); break;
					case '3': sReturn.append ("0011"); break;
					case '4': sReturn.append ("0100"); break;
					case '5': sReturn.append ("0101"); break;
					case '6': sReturn.append ("0110"); break;
					case '7': sReturn.append ("0111"); break;
					case '8': sReturn.append ("1000"); break;
					case '9': sReturn.append ("1001"); break;
					case 'a': sReturn.append ("1010"); break;
					case 'b': sReturn.append ("1011"); break;
					case 'c': sReturn.append ("1100"); break;
					case 'd': sReturn.append ("1101"); break;
					case 'e': sReturn.append ("1110"); break;
					case 'f': sReturn.append ("1111"); break;
				}
			}
			return sReturn;
		}
//this function changes binary string to decimal integer value
int cache::bintodec(string bin, int binlength)//input- binary string,length of binary string (number of bits)
{
    int i;
    long long int a=0;
	for(i=binlength-1; i>-1;i--) // for all bits in the binary string
        {
            if(bin[i]=='1')
            {
                a=a+pow(2,binlength-1-i);  // binary to decimal conversion
            }
        }
return a; // returns the converted decimal value
}

int main(int argc, char*argv[])  //number of parameters and the array of parameters
{
     char tagarray[64];        // array to hold the tag part of the address (binary string)
     char rw = 0;              // incoming request
     char indexarray[64];      // array to hold index value (bin string)
     int index;                // index value
     int tagsize =0;           // number of bits for tag
     int address_size = 0;     // number of bits in incoming address
     string bin; //bin string
     string hex; //hex string
     string line;//

    blocksize=atoi(argv[1]);    // convert blocksize from string to int
    cache_size = atoi(argv[2]); // cache size
    assoc = atoi(argv[3]);      // associativity
    replace_policy = atoi(argv[4]); // lru = 0 , lfu = 1
    write_policy = atoi(argv[5]);   // wbwa = 0, wtna = 1
    char *trace_file = argv[6];     // trace file name

    sets = cache_size/ (blocksize* assoc); //calculate number of sets
    int block_offsetsize = log2(blocksize);//number of bytes in the block (number of bits from address)
    int indexsize = log2(sets); // number of sets (number of bits for index)
    cout<<"  ===== Simulator configuration ====="<<endl;
    cout<<"  L1_BLOCKSIZE:       "<<atoi(argv[1])<<endl;
    cout<<"  L1_SIZE:            "<<atoi(argv[2])<<endl;
    cout<<"  L1_ASSOC:           "<<atoi(argv[3])<<endl;
    cout<<"  trace_file:         "<<trace_file<<endl;
    if (replace_policy==0)
    cout<<"  Replacement Policy: "<<"LRU"<<endl;
    else if (replace_policy==1)
    cout<<"  Replacement Policy: "<<"LFU"<<endl;

    cache L1(sets, assoc); //create object for L1 cache and pass parameters sets and associativity = rows and columns in 2D array
    unsigned long long int address_count =0; //keep track of number of incoming address

    ifstream file(trace_file); //read the incoming file
    if (file.is_open())
		{
			while (getline(file,line)) //while in first line of the input file
			{
			   address_count++; // increment the number of addresses
               cout<<"#"<<"address_count"<<"\t: "<<std::dec<<address_count<<endl;
               address_size = ((line.length()-2)*4); //calculate number of bits in the address 32 or 64
               // cout<<"address_size"<<"\t:"<< address_size<<endl;
               bin = L1.hex_char_to_bin(line); //convert hex string to bin string
               rw = line[0]; //the first character in the line in input file is read/write
			   tagsize = address_size - indexsize - block_offsetsize; //number of bits for tag
               //cout<<"tag_size"<<"\t:"<< tagsize<<endl;

               for(int i=0; i<tagsize; i++)  //store tag part of the address in an array
                {
                    tagarray[i] = bin[i];    //array to hold the tag part of the address
                }
               L1.tag = L1.bintodec(tagarray, tagsize); //convert binary tag to decimal value, to use for comparisons

               for(int i=tagsize;i< tagsize + indexsize; i++) //store index part of the address in an array
                {
                    indexarray[i-tagsize] = bin[i];  //array to hold the index part of the address
                }
               L1.index = L1.bintodec(indexarray,indexsize); //convert binary index to decimal value, to use for comparisons

               if (rw == 'r'| rw == 'R') //when processor sends read request
                {
                    L1.r_count++;        //increment the read count
                    L1.read(L1.tag, L1.index); //call the read function which takes parameters tag and index (tag value and cache line number)
                }
                else if(rw == 'w' | rw == 'W') //when processor sends write request
                {
                    L1.w_count++;        //increment the write count
                    L1.write(L1.tag, L1.index); //call the write function which takes parameters tag and index (tag value and cache line number)
                }
                else
                cout<<"Error:Expected read/write command not found.";

       cout<<"  ==============================================================="<<endl;
	   cout<<endl;
       cout<<"current processor request: "<<line<<endl;
       cout<<"  ========================== L1 contents ========================"<<endl;

       for(int j =0;j<sets;j++)
        {
            cout<<"set"<<"\t";
            cout<<std::dec<<j<<":\t";
            for(int i=0;i<assoc;i++)
            {
                std::cout<<std::hex<<L1.tag_store[j][i];
                if(write_policy==0 && L1.dirty_bit[j][i]==1)
                    cout<<"D";
                    cout<<"\t\t\t";
            }cout<<endl;
         }
        cout<<"  ==============================================================="<<endl;
             cout<<endl;
             getchar();
             system("cls");
			}
        double numberofmisses = (L1.r_miss_count+L1.w_miss_count);
        cout<< std::dec<<numberofmisses <<endl;
        double numberofaccessess = (L1.r_count+L1.w_count);
        cout << std::dec<<numberofaccessess<<endl;
        double missrate;
        missrate = (numberofmisses/numberofaccessess)*100;
        cout<<std::dec<<missrate<<endl;
        cout<<"reads          "<<std::dec<<L1.r_count<<endl;
        cout<<"write          "<<std::dec<<L1.w_count<<endl;
        cout<<"read_hit       "<<std::dec<<L1.r_hit_count<<endl;
        cout<<"write-hit      "<<std::dec<<L1.w_hit_count<<endl;
        cout<<"read_miss      "<<std::dec<<L1.r_miss_count<<endl;
        cout<<"write-miss     "<<std::dec<<L1.w_miss_count<<endl;
        cout<<"write_back?    "<<std::dec<<L1.wb_count<<endl;
        cout<<"Miss rate = Number of misses/ Number of accesses =  " <<std::dec<< missrate<<endl;
}
    file.close();
    return 0;
}

int cache::read(unsigned long long int tag,unsigned long long int index) //parameters(incoming tag value and cache line number)
{
    int evict_block=0; //initialize the block to be evicted as 0
    for(int j=0;j<assoc;j++) //scan all columns
    {
        if(tag_store[index][j] == tag) //is tag value of any block(column) matching the incoming tag address
        {
           r_hit_count++; // increment hit count if yes
           update_counter(index,j,1);//update LRU and LFU. parameters(row,column).(1 == hit)
           return 1;
        }
    }//miss
    r_miss_count++; //else increment miss count
    for(int i =0;i<assoc;i++) //scan all columns
    {
         if (valid_bit[index][i] == 0) //cache is newly being updated(all valid bits are zero so far)
         {
             tag_store[index][i] = tag; //keep the incoming tag in the tagstore for that block
             update_counter(index,i,0); //update LRU and LFU. parameters(row,column).(0 == miss)
             return 0;
         }
    }
    //check how many blocks are valid
    int x=0; //number of blocks
    for(int k=0;k<assoc;k++)
         {
              if(valid_bit[index][k]==1)
                {
                        x++; // increment number of blocks that are valid
                }
         }

              if(x==assoc) // when a miss and if all blocks in the set are valid,
                {
                evict_block = evictblock(); // get which block to evict in that set (block number)
                if (write_policy == 0) //WBWA
                {
                    if(dirty_bit[index][evict_block]==1) //dirty bit of the block to evict is 1
                    {
                        wb_count++; //write back to memory, keep track of write back count
                    }
                    dirty_bit[index][evict_block]=0;  //make dirty bit 0 now, since we have written back old data to memory
                }
                //else WTNA - do nothing
                update_counter(index,0,0);
                tag_store[index][evict_block]= tag;
                valid_bit[index][evict_block] =1; //safeside
                return 0;
                }
}

int cache:: write(unsigned long long int tag,unsigned long long int index)
{

    int evict_block=0; //initialize the block to be evicted as 0
    for(int j=0;j<assoc;j++)
    {
        if(tag_store[index][j] == tag) //is tag value of any block(column) matching the incoming tag address
        {
            w_hit_count++; // increment hit count if yes
            if(write_policy == 0)//WBWA
                dirty_bit[index][j]=1; //because WA, we allocate cache block for this data, and make it dirty
            //else  for WTNA do nothing as dirty bit is initialized as 0
             int k= update_counter(index,j,1);//update LRU counter
            return 1;
        }
    } //miss
    w_miss_count++;
    for(int i=0;i<assoc;i++)
        {
        if(valid_bit[index][i]==0)//cache is newly being updated(all valid bits are zero so far)
        {
            if(write_policy == 0)//WBWA
            {
                tag_store[index][i]= tag; //keep the incoming tag in the tagstore for that block
                dirty_bit[index][i] = 1;  //set dirty bit, because data inconsistent with data at this address in the memory
                update_counter(index,i,0);//miss case
                valid_bit[index][i] = 1;
            }
            return 0;
        }
        }   //check how many blocks are valid
            int x=0; //number of blocks
            for(int k=0;k<assoc;k++)
            {
                if(valid_bit[index][k]==1)
                {
                        x++; // increment number of blocks that are valid
                }
            }

            if(x==assoc) // when a miss and if all blocks in the set are valid,
            {
                evict_block = evictblock();// get which block to evict in that set (block number)
                    if (write_policy == 0)//WBWA
                    {
                        if(dirty_bit[index][evict_block]==1) //dirty bit of the block to evict is 1
                        {
                            wb_count++; //write back to memory, keep track of write back count
                        }
                        tag_store[index][evict_block]= tag;
                        update_counter(index,0,0);
                        dirty_bit[index][evict_block] = 1;
                    }
                    return 0;
            }
}
int cache:: update_counter(unsigned long long int index, int k,int hit) //(row,column,hit/miss)
{
    unsigned long long int large=0;
    if(replace_policy ==0) //lRU
    {
       large = lru_count[index][k]; //set large as the referenced blocks' LRU counter value

        if(hit ==1)//hit
        {
            for(int i=0;i<assoc;i++) //scan all columns
            {
                if(lru_count[index][k]!=0) //If referenced blocks' LRU is non zero
                {
                    if(lru_count[index][i]<large) //if any other blocks' LRU counter is smaller,
                    lru_count[index][i]++;   // increment their LRU counter by 1
                }
            }
            lru_count[index][k] = 0; //set the referenced block's LRU counter value as 0
        }
        else //miss
        {
            if(valid_bit[index][k] == 0) //when the referenced block is filled for the first time in that cache line
            {
                for(int i=0;i<assoc;i++) //scan all columns
                {
                    lru_count[index][i]++; // and increment LRU counter of all blocks in that row
                }
                lru_count[index][k]=0; // but make the LRU counter of the referenced block 0
            }
            else
            {

            for(int i =0;i<assoc; i++) //scan all columns
            {   //try to find the block which has the highest LRU counter value
                if(large < lru_count[index][i])
                {
                    large = lru_count[index][i];
                    k=i; // make that the current referenced block
                }
            }
            for(int i=0;i<assoc;i++)
            {
                lru_count[index][i]++; //increment all other blocks LRU counter value
            }
            lru_count[index][k]=0; // and make the LRU counter value of the current referenced block as 0
            }
         }
    }
    else //replace_policy = LFU
    {
        if(hit==1) //hit
        {
            lfu_count[index][k]++; // increment current referenced blocks LFU counter
        }
        else //miss
        {
            if(valid_bit[index][k] == 0) //when the referenced block is filled for the first time in that cache line
            {
                lfu_count[index][k]= age[index][k]+1; //
            }
            else
            {
            int lowest = lfu_count[index][k]; //set lowest as the referenced blocks' LFU counter value
                for(int i=0;i<assoc;i++) //scan all columns
                {
                    if(lowest> lfu_count[index][i])
                    {//try to find the block which has lowest LFU counter value
                        lowest = lfu_count[index][i];
                        k=i; // make that the current referenced block
                    }

                }
            age[index][k] = lfu_count[index][k]; //make age = current referenced block's LFU counter
            lfu_count[index][k]=age[index][k]+1; //current referenced block's LFU counter = age+1
            }
    }
}
return k; //currently referenced blocks' column
}
int cache:: evictblock()
{
int large =0,k=0;
int least=lfu_count[index][0];
    if(replace_policy ==0) //LRU
    {
        for(int i=0;i<assoc;i++)//scan all columns
        { // find the block with the largest LRU value
         if(large<lru_count[index][i])
         {
          large = lru_count[index][i];
          k=i; //number of the block with the largest LRU value
         }
    }
    }
    else
    {
        for(int i=0;i<assoc;i++)
        {// find the block with the least LFU counter value
        if(least>lfu_count[index][i])
        {
        least = lfu_count[index][i];
        k=i; //number of the block with the least LFU value
        }
    }
}
 return k;
}
