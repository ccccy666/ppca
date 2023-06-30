#include<bits/stdc++.h>
using std::cin;
using std::string;
using std::cout;
int mask0_4 = 0b0000000'00000'00000'000'00000'0011111;
int mask0_6 = 0b0000000'00000'00000'000'00000'1111111;
int mask0_7 = 0b0000000'00000'00000'000'00001'1111111;
int mask8_15=  0b0000000'00000'00001'111'11110'0000000;
int mask0_15 = 0b0000000'00000'00001'111'11111'1111111;
int mask16_23= 0b0000000'01111'11110'000'00000'0000000;
int mask24_31= 0b1111111'10000'00000'000'00000'0000000;
int mask7_11 = 0b0000000'00000'00000'000'11111'0000000;
int mask12_14 = 0b0000000'00000'00000'111'00000'0000000;
int mask15_19 = 0b0000000'00000'11111'000'00000'0000000;
int mask20_24 = 0b0000000'11111'00000'000'00000'0000000;
int mask25_31 = 0b1111111'00000'00000'000'00000'0000000;
int mask8_11 = 0b0000000'00000'00000'000'11110'0000000;//only used in SB commands
int mask25_30 = 0b0111111'00000'00000'000'00000'0000000;//only used in SB commands
int mask21_30 = 0b0111111'11110'00000'000'00000'0000000;//only used in UJ commands
int mask12_31 = mask12_14 | mask15_19 | mask20_24 | mask25_31, mask20_31 = mask20_24 | mask25_31, mask12_19 =
        mask12_14 | mask15_19;
int cycle=0,PC=0;
bool stuck=0;
int rs_num=0,sl_num=0;
int jumpnum=0;
int cnt=0;
int sign_expansion(int x,int digit){
    //return x >> beg  ? -1 ^ (1 << beg) - 1 | x : x;
    if(digit==0)return 0;
    if(digit==32)return x ;
    bool high=(x&(1<<(digit-1)));
    if(high){
        x=(x|(0xffffffff<<digit));
    }
    return x;
}
enum abstract{
    U,
    J,
    I,
    S,
    B,
    R,
    err,
};
enum  CmdType{
    invalid,
    lb,lh,lw,lbu,lhu,
    sb,sh,sw,
    add,addi,
    sub,
    lui,
    auipc,
    XOR,xori,OR,ori,AND,andi,
    sll,slli,srl,srli,sra,srai,
    slt,slti,sltu,sltiu,
    beq,bne,blt,bge,bltu,bgeu,jal,jalr
};

struct Command{
    CmdType type=CmdType::invalid;
    abstract tpe;

    uint32_t rd=0,rs1=0,rs2=0,imm=0;
    int32_t rv1=0,rv2=0;
    Command(){};
    Command(CmdType t,int d,int s1,int s2,int im,int v1,int v2,abstract ab):type(t),rd(d),rs1(s1),rs2(s2),imm(im),rv1(v1),rv2(v2),tpe(ab){}
};
enum memop{
    FP,LOAD,STORE,JUMP,NONE
};
memop get_memop(Command &x)
{
    switch (x.type)
    {
        case lb:
        case lh:
        case lw:
        case lbu:
        case lhu:
            return LOAD;
        case sb:
        case sh:
        case sw:
            return STORE;
        case jal:
        case jalr:
        case beq:
        case bne:
        case blt:
        case bge:
        case bltu:
        case bgeu:
            return JUMP;
        default:
            return FP;
    }
}
class predictor{
public:
    int sta[4]={0,1,2,3};
    int state;
    predictor(){
        state=3;
    }
    bool change(bool jump){
        if(state==0){
            if(jump){
                state=1;
            }
        }else if(state==1){
            if(jump){
                state=2;
            }else{
                state=0;
            }
        }else if(state==2){
            if(jump){
                state=3;
            }else{
                state=1;
            }
        }else{
            if(!jump){
                state=2;
            }
        }

    }
    bool get_state(){
        return state>1;
    }
};
predictor pre[6];
int get_pre(Command cmd)
{
    switch (cmd.type)
    {
        case beq:return 0;
        case bne:return 1;
        case blt:return 2;
        case bge:return 3;
        case bltu:return 4;
        case bgeu:return 5;

    }

}
class memory{
public:
    int data[100000];
    int pos=0;
    memory(){
        memset(data,0,sizeof(data));
    }
    ~memory(){};

    void get_all(){
        string line;
        while(getline(cin,line)){
            //cout<<line<<endl;
            if(line[0]=='@'){
                string sub=line.substr(1);
                pos=std::stoi(sub,nullptr,16);
            }else{
                //int order=0;
                //int ct=0;
                for(int i=0;i<line.size();i+=3){
                    string sub=line.substr(i,2);
                    data[pos++]=(std::stoi(sub, nullptr,16));
                    //ct++;
                    //if(ct==4){
                        //=order;
                        //order=0;
                        //ct=0;
                   // }
                }

            }
        }
    }

};
memory mem;;


class reorder{
public:
    int sequence;
    int tag;
    bool busy;
    bool ready;
    bool occupy;
    bool depend;
    bool jump;
    int time;
    int dest;
    int value;
    int vj,vk,qj,qk,record,A;
    memop totype;
    Command cmd;
    reorder(){
        sequence=value=dest=time=tag=0;
        vj=vk=qj=qk=record=A=0;
        busy=ready=occupy=jump=0;
        depend=1;
        cmd=Command();
        totype=NONE;
    };
    ~reorder(){};

};

class ROB{//reordered buffer
public:
    int head=0,tail=0;
    reorder buffer[10000];
    int size=0;
    //ROB(){};
    ROB(int t=32){
        head=tail=0;
        size=t;
    }
    ~ROB(){};
    bool empty(){
        return head==tail;
    }
    bool full(){
        return (tail+1)%size==head;
    }
    reorder &front(){
        return buffer[(head+1)%size];
    }
    void push(reorder &re){
        if(!full()){
            tail=(tail+1)%size;
            buffer[tail]=re;
            //return true;
        }
       // return false;
    }
    void pop(){
        if(!empty()){
            head=(head+1)%size;
            buffer[head]=reorder();
        }
    }
};
reorder rs[70];
ROB rob,queue;

class regist{
public:
    int data[33];
    int reorder[33];
    regist(){
        memset(data,0,sizeof(data));
        memset(reorder,0,sizeof(reorder));
    };
    ~regist(){};

};
regist reg;

bool predict(reorder &input)
{
    return pre[get_pre(input.cmd)].get_state();
        //return His_Pre[get_reversetype(ip.cmd)].get_prediction(branch[get_reversetype(ip.cmd)].st);
}
int to_rs(reorder &input){
    if(input.totype==LOAD||input.totype==STORE){
        for(int i=33;i<=64;i++){
            if(!rs[i].occupy){
                rs[i]=input;
                rs[i].occupy=1;
                sl_num++;
                rob.buffer[input.tag].tag=i;

                return i;
            }


        }
    }else{
        for(int i=1;i<=32;i++){
            if(!rs[i].occupy){
                rs[i]=input;
                rs[i].occupy=1;
                rs_num++;
                rob.buffer[input.tag].tag=i;

                return i;
            }

        }
    }
}
void pushtoRS(reorder &input){
    if(rob.full())return;
    if((input.totype==LOAD||input.totype==STORE)){
        if(sl_num==32)return;
    }else{
        if(rs_num==32)return;
    }

    int rs1=input.cmd.rs1,rs2=input.cmd.rs2;
    switch(input.cmd.type){
        case lui:
        case auipc:
        case jal:
            break;
        case jalr:
        case lb:
        case lh:
        case lw:
        case lbu:
        case lhu:
        case addi:
        case slti:
        case sltiu:
        case xori:
        case ori:
        case andi:
        case slli:
        case srli:
        case srai:
            if(reg.reorder[rs1]){
                //input.
                input.qj=reg.reorder[rs1];
            }else input.vj=reg.data[rs1];
            break;
        default:
            if(reg.reorder[rs1]){
                input.qj=reg.reorder[rs1];
            }else input.vj=reg.data[rs1];
            if(reg.reorder[rs2]){
                input.qk=reg.reorder[rs2];
            }else input.vk=reg.data[rs2];
            break;
    }
    rob.push(input);
    input.tag=rob.tail;
    //cout<<rs1<<' '<<rs2<<' '<<rob.tail<<' '<<input.A<<' '<<input.dest<<' '<<input.sequence<<' ';
    to_rs(input);
    switch(input.cmd.tpe){
        case R:
        case I:
        case U:
        case J:
            reg.reorder[input.dest]=rob.buffer[rob.tail].tag;
    }

    queue.pop();

}

reorder ID(int cmd){
    reorder ord;
    int op, func3, func7, rd, rs1, rs2,imm=0;
    //cmd =IF();
    Command com;

    op = cmd & mask0_6, func3 = (cmd & mask12_14) >> 12, func7 = (cmd & mask25_31) >> 25;
    rd =(cmd & mask7_11) >> 7, rs1 = (cmd & mask15_19) >> 15, rs2 = (cmd & mask20_24) >> 20;
    switch (op) {
        //imm=cmd&mask12_31;
        case 0b0110111://lui
            imm=cmd&mask12_31;
            com=Command(lui,rd,0,0,imm,0,0,U);
            ord.vk=PC;
            ord.dest=rd;
            break;
        case 0b0010111://auipc
            imm=cmd&mask12_31;
            com=Command(auipc,rd,0,0,imm,0,0,U);
            ord.vk=PC;
            ord.dest=rd;
            break;
        case 0b1101111://jal
            
            imm=(cmd&mask12_19)|((cmd&(1<<20))>>9)|((cmd&mask21_30)>>20)|((cmd&(1<<31))>>11);
            imm= sign_expansion(imm,21);
            com=Command(jal,rd,0,0,imm,0,0,J);
            ord.vk=PC;
            ord.dest=rd;
            break;
        case 0b1100111://jalr
            imm=cmd>>20;
            
            imm= sign_expansion(imm,12);
            com=Command(jalr,rd,rs1,0,imm,reg.data[rs1],0,I);
            ord.vk=PC;
            ord.dest=rd;
            break;
        case 0b1100011://B
            imm=((cmd&mask25_30)>>20)|((cmd&(1<<31))>>19)|((cmd&mask8_11)>>7)|((cmd&(1<<7))<<4);
            imm= sign_expansion(imm,13);

            switch (func3) {
                case 0b000://beq
                    com=Command(beq,0,rs1,rs2,imm,reg.data[rs1],reg.data[rs2],B);
                    break;
                case 0b001://bne
                    //imm=cmd>>25;
                    com=Command(bne,0,rs1,rs2,imm,reg.data[rs1],reg.data[rs2],B);
                    break;
                case 0b100://blt
                    //imm=cmd&mask12_31;
                    com=Command(blt,0,rs1,rs2,imm,reg.data[rs1],reg.data[rs2],B);
                    break;
                case 0b101://bge
                    //imm=cmd&mask12_31;
                    com=Command(bge,0,rs1,rs2,imm,reg.data[rs1],reg.data[rs2],B);
                    break;
                case 0b110://bltu
                    //imm=cmd&mask12_31;
                    com=Command(bltu,0,rs1,rs2,imm,reg.data[rs1],reg.data[rs2],B);
                    break;
                case 0b111://bgeu
                    //imm=cmd&mask12_31;
                    com=Command(bgeu,0,rs1,rs2,imm,reg.data[rs1],reg.data[rs2],B);
                    break;
            }
            break;
        case 0b0000011://lb,lh,lw...
            imm=cmd>>20;
            imm= sign_expansion(imm,12);
            ord.dest=rd;
            switch (func3) {
                case 0b000://lb
                    //imm=cmd&mask12_31;
                    com=Command(lb,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b001://lh
                    //imm=cmd&mask12_31;
                    com=Command(lh,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b010://lw
                    //imm=cmd&mask12_31;
                    com=Command(lw,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b100://lbu
                    //imm=cmd&mask12_31;
                    com=Command(lbu,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b101://lhu
                    //imm=cmd&mask12_31;
                    com=Command(lhu,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;

            }
            break;
        case 0b0100011://S
            imm=((cmd&mask7_11)>>7)|((cmd&mask25_31)>>20);
            imm= sign_expansion(imm,12);

            switch (func3) {
                case 0b000://sb
                    //imm=cmd&mask12_31;
                    com=Command(sb,0,rs1,rs2,imm,reg.data[rs1],reg.data[rs2],S);
                    break;
                case 0b001://sh
                    //imm=cmd&mask12_31;
                    com=Command(sh,0,rs1,rs2,imm,reg.data[rs1],reg.data[rs2],S);
                    break;
                case 0b010://sw
                    //imm=cmd&mask12_31;
                    com=Command(sw,0,rs1,rs2,imm,reg.data[rs1],reg.data[rs2],S);
                    break;
            }
            break;
        case 0b0010011://I
            imm=cmd>>20;
            imm= sign_expansion(imm,12);
            //ord.dest=rd;
            ord.dest=rd;
            switch (func3) {
                case 0b000://addi
                    //imm=cmd&mask12_31;
                    com=Command(addi,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b010://slti
                    //imm=cmd&mask12_31;
                    com=Command(slti,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b011://sltiu
                    //imm=cmd&mask12_31;
                    com=Command(sltiu,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b100://xori
                    //imm=cmd&mask12_31;
                    com=Command(xori,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b110://ori
                    //imm=cmd&mask12_31;
                    com=Command(ori,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b111://andi
                    //imm=cmd&mask12_31;
                    com=Command(andi,rd,rs1,0,imm,reg.data[rs1],0,I);
                    break;
                case 0b001://slli
                    //imm=cmd&mask;
                    com=Command(slli,rd,rs1,0,rs2,reg.data[rs1],0,I);
                    break;
                case 0b101:
                    switch (func7) {
                        case 0b0000000://srli
                            //imm=cmd&mask12_31;
                            com=Command(srli,rd,rs1,0,rs2,reg.data[rs1],0,I);
                            break;
                        case 0b0100000://srai
                            //imm=cmd&mask12_31;
                            com=Command(srai,rd,rs1,0,rs2,reg.data[rs1],0,I);
                            break;
                    }
                    break;
                default:
                    com=Command(invalid,0,0,0,0,0,0,I);
                    break;

            }
            break;
        case 0b0110011://R
            ord.dest=rd;
            switch (func3) {
                case 0b000:
                    switch (func7) {
                        case 0b0000000://add
                            //imm=cmd&mask12_31;
                            com=Command(add,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                            break;
                        case 0b0100000://sub
                            //imm=cmd&mask12_31;
                            com=Command(sub,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                            break;
                    }
                    break;
                case 0b001://sll
                    //imm=cmd&mask12_31;
                    com=Command(sll,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                    break;
                case 0b010://slt
                    //imm=cmd&mask12_31;
                    com=Command(slt,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                    break;
                case 0b011://sltu
                    //imm=cmd&mask12_31;
                    com=Command(sltu,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                    break;
                case 0b100://xor
                    //imm=cmd&mask12_31;
                    com=Command(XOR ,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                    break;
                case 0b101:
                    switch (func7) {
                        case 0b0000000://srl
                            //imm=cmd&mask12_31;
                            com=Command(srl,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                            break;
                        case 0b0100000://sra
                            //imm=cmd&mask12_31;
                            com=Command(sra,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                            break;
                    }
                    break;
                case 0b110://or
                    //imm=cmd&mask12_31;
                    com=Command(OR,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                    break;
                case 0b111://and
                    //imm=cmd&mask12_31;
                    com=Command(AND,rd,rs1,rs2,0,reg.data[rs1],reg.data[rs2],R);
                    break;

            }
            break;
        default:
            ord.dest=-1;
            return ord;
    }
    //cout<<rs1<<' '<<rs2<<' '<<(unsigned int)imm<<std::endl;
    ord.sequence=cmd;
    ord.A=imm;
    
    if(com.tpe==B){
        ord.dest=PC;
        ord.jump= predict(ord);
        if(ord.jump)PC=ord.dest+ord.A-4;
    }
    
    ord.totype= get_memop(com);
    if(ord.totype==LOAD||ord.totype==STORE){
        ord.time=3;
    }else ord.time=1;
    ord.cmd=com;
    // cout<<rs1<<' '<<rs2<<std::endl;
    // cout<<com.rs1<<' '<<com.rs2<<std::endl;
    return ord;
}
void IF(){
    if(stuck)return;
    int x=0;
    for(int i=3;i>=0;i--){
        x=(x<<8)|(mem.data[PC+i]);
        //cout<<mem.data[PC+i]<<' ';
    }
    reorder re=ID(x);
    //cout<<re.cmd.rs1<<' '<<re.cmd.rs2<<std::endl;
    if(re.dest!=-1){
        //std::cout<<x<<std::endl;
        queue.push(re);
}
}

void execute(reorder &input){
    switch(input.cmd.type){
        case lui:
            break;
        case auipc:
        case jal:
            input.A+=input.vk;//vk就是pc
            break;
        case jalr:
            input.A=input.vj+(input.A&(~1));
            break;
        case beq:
        case bne:
        case blt:
        case bge:
        case bltu:
        case bgeu:
            input.A+=input.dest;//dest是pc
            break;

        case lb:
        case lh:
        case lw:
        case lbu:
        case lhu:
        case sb:
        case sh:
        case sw:
        case addi:
            input.A+=input.vj;
            break;
        case slti:
            input.A=(input.vj<input.A);
            break;
        case sltiu:
            input.A=((unsigned int)input.vj<(unsigned int)input.A);
            break;
        case xori:
            input.A^=input.vj;
            break;
        case ori:
            input.A|=input.vj;
            break;
        case andi:
            input.A&=input.vj;
            break;
        case slli:
            input.A=input.vj<<(input.A&mask0_4);
            break;
        case srli:
            input.A=((unsigned int)input.vj>>(input.A&mask0_4));

            break;
        case srai:{
            //int from=input.A&mask0_4;
            input.A=input.vj>>(input.A&mask0_4);
            //input.A=sign_expansion(input.A,32-from);
            break;
        }
        case add:

            input.A=input.vk+input.vj;
            break;
        case sub:
            input.A=input.vj-input.vk;

            break;
        case sll:
            input.A=input.vj<<(input.vk&mask0_4);
            break;
        case slt:
            input.A=(input.vj<input.vk);
            break;
        case sltu:
            input.A=((unsigned int)input.vj<(unsigned int)input.vk);
            break;

        case XOR:
            input.A=input.vj^input.vk;
            break;
        case srl:
            input.A=((unsigned int)input.vj>>(input.vk&mask0_4));
            break;
        case sra:
            input.A=input.vj>>(input.vk&mask0_4);
            //input.A=sign_expansion(input.A,32-(input.vk&mask0_4));
            break;
        case OR:
            input.A=input.vj|input.vk;
            break;
        case AND:
            input.A=input.vj&input.vk;
            break;
    }
}

void wb(reorder &input){
    rob.buffer[input.tag].busy=true;
    rob.buffer[input.tag].A=input.A;
    rob.buffer[input.tag].vj=input.vj;
    rob.buffer[input.tag].vk=input.vk;
    switch(input.cmd.type){
        case beq:
            if(input.vj!=input.vk)rob.buffer[input.tag].depend=0;
            break;
        case bne:
            if(input.vj==input.vk)rob.buffer[input.tag].depend=0;

            break;
        case blt:
            if(input.vj>=input.vk)rob.buffer[input.tag].depend=0;

            break;
        case bge:
            if(input.vj<input.vk)rob.buffer[input.tag].depend=0;

            break;
        case bltu:
            if((unsigned int)input.vj>=(unsigned int)input.vk)rob.buffer[input.tag].depend=0;

            break;
        case bgeu:
            if((unsigned int)input.vj<(unsigned int)input.vk)rob.buffer[input.tag].depend=0;

            break;

    }
}
void commit(reorder &input){
    //cout<<input.vj<<' '<<input.A<<' '<<PC<<' '<<input.sequence<<std::endl;
    
    switch(input.cmd.type){
        
        case jal:
        case jalr:
            reg.data[input.dest]=input.vk+4;
            PC=input.A;
            break;
        case lb:
            reg.data[input.dest]= sign_expansion(mem.data[input.A],8);
            //sign_expansion()
            break;
        case lh:
            reg.data[input.dest]=mem.data[input.A+1];
            reg.data[input.dest]=((reg.data[input.dest]<<8)|mem.data[input.A]);
            reg.data[input.dest]= sign_expansion(reg.data[input.dest],16);
            break;
        case lw:
            reg.data[input.dest]=0;
            for(int i=3;i>=0;i--){
                //cout<<mem.data[input.A+i]<<' ';
                reg.data[input.dest]=((reg.data[input.dest]<<8)|mem.data[input.A+i]);
            }
            break;
        case lbu:
            reg.data[input.dest]=mem.data[input.A];
            break;
        case lhu:{
            reg.data[input.dest]=mem.data[input.A+1];
            reg.data[input.dest]=((reg.data[input.dest]<<8)|mem.data[input.A]);
            break;
        }
        
        case sb:
            
            mem.data[input.A]=input.vk&mask0_7;
            break;
        case sh:
            
            mem.data[input.A]=input.vk&mask0_7;
            mem.data[input.A+1]=(input.vk&mask8_15)>>8;;
            break;
        case sw:
            //std::cout<<111111;
            
            mem.data[input.A]=input.vk&mask0_7;
            mem.data[input.A+1]=(input.vk&mask8_15)>>8;
            mem.data[input.A+2]=(input.vk&mask16_23)>>16;
            mem.data[input.A+3]=(input.vk&mask24_31)>>24;
            //cout<<mem.data[input.A+1]<<' '<<input.vk<<' '<<7777777777<<' ';
            break;
        case beq:
        case bne:
        case blt:
        case bge:
        case bltu:
        case bgeu:
            jumpnum++;
            if(!input.depend&&input.jump)//not Jump
                PC=input.dest+4;
            if(input.depend&&!input.jump)// Jump
                PC=input.A;
            pre[get_pre(input.cmd)].change(input.depend);
//            His_Pre[get_reversetype(ip.cmd)].change_history_state(branch[get_reversetype(ip.cmd)].st,!(ip.solve==ip.prJump));
//            branch[get_reversetype(ip.cmd)].upd(!ip.solve);
            break;
        default:
            reg.data[input.dest]=input.A;
            break;
    }
}

void cdb(int x) {
    for (int i = 1; i <= 64; i++) {
        if (rs[i].qj == x) {
            rs[i].qj = 0;
            if (rs[x].cmd.type == jal || rs[x].cmd.type == jalr) {
                rs[i].vj = rs[x].vk + 4;//vk就是pc
            } else rs[i].vj = rs[x].A;

            
        }
        if (rs[i].qk == x) {
                rs[i].qk = 0;
                if (rs[x].cmd.type == jal || rs[x].cmd.type == jalr) {
                    rs[i].vk = rs[x].vk + 4;
                } else rs[i].vk = rs[x].A;
            }
    }
}
void printrob()
    {
        for(int i=(rob.head+1)%rob.size;i!=(rob.tail+1)%rob.size;i=(i+1)%rob.size)
            printf("[IN ROB]number:%d time:%d solve:%d busy:%d type:%c tomatype:%c qj:%u qk:%u rd:%u imm:%d\n",rob.buffer[i].tag,rob.buffer[i].time,rob.buffer[i].depend?1:0,rob.buffer[i].busy?1:0,0,0,rob.buffer[i].qj,rob.buffer[i].qk,rob.buffer[i].dest,(int)rob.buffer[i].A);
    }
void printreg()
    {
        for(int i=0;i<32;++i)
            printf("%2d ",i);
        printf("\n");
        for(int i=0;i<32;++i)
            printf("%2u ",reg.data[i]);
        printf("\n");
        for(int i=0;i<32;++i)
            printf("%2u ",reg.reorder[i]);
        printf("\n");
    }
    void printrs(){
        for(int i=1;i<=64;i++){
            std::cout<<rs[i].vj<<' '<<rs[i].vk<<' '<<rs[i].qj<<' '<<rs[i].qk<<' '<<rs[i].A<<'\n';
        }
    }
int run(){
    //cout<<1<<std::endl;
    bool commi=0;
    stuck=queue.full();
    if(!stuck)IF();
    if(!rob.empty()&&rob.front().busy)rob.front().time--;
    if(!rob.empty()&&rob.front().busy&&!rob.front().time){
        commi=1;
        //std::cout<<rob.front().sequence<<' '<<(int)0x0ff00513<<std::endl;
        if(rob.front().sequence==(int)0x0ff00513){
            //cout<<99999999999<<std::endl;
            return (unsigned int)reg.data[10]&mask0_7;
        }
        commit(rob.front());//写回
        switch(rob.front().cmd.tpe){
            case I:
            case R:
            case U:
            case J:
                if(reg.reorder[rob.front().dest]==rob.front().tag){
                    reg.reorder[rob.front().dest]=0;
                }
                break;
        }
        reg.data[0]=reg.reorder[0]=0;
    }
    int cdb_=0;
    for(int i=1;i<=64;i++){//遍历rs
        if(!rs[i].busy&&!rs[i].depend){
            //std::cout<<i<<' '<<6666666<<std::endl;
            wb(rs[i]);
            cdb_=i;
            rs[i].busy=1;//防止重复广播
            break;
        }
    }
    for(int i=1;i<=64;i++){//处理
        if(!rs[i].depend||!rs[i].occupy)continue;
        if(rs[i].qj==0&&rs[i].qk==0&&!rs[i].busy)rs[i].busy=1;
        if(rs[i].busy){
            //std::cout<<i<<'\n';
            execute(rs[i]);
            //cout<<rs[i].A<<' ';
            rs[i].depend=0;
            rs[i].busy=0;
            //continue;
        }

    }
    //cout<<std::endl;
    if(!queue.empty())pushtoRS(queue.front());
    if(cdb_>0&&cdb_<=32)cdb(cdb_);//广播
    if(commi){//当前周期有commit
        if(rob.front().totype==LOAD||rob.front().totype==STORE){
            int i=rob.front().tag;
            
            for(int j=1;j<=64;++j)
            {
                if(rs[j].qj==i)
                {//std::cout<<i<<' ';
                    
                    rs[j].qj=0;
                    rs[j].vj=reg.data[rob.front().dest];
                }
                if(rs[j].qk==i)
                {
                    rs[j].qk=0;
                    rs[j].vk=reg.data[rob.front().dest];
                }
            }
            sl_num--;
        }else{
            cdb(rob.front().tag);
            rs_num--;
        }
        rs[rob.front().tag]=reorder();
        if((rob.front().cmd.tpe==B&&rob.front().depend!=rob.front().jump)||rob.front().cmd.type==jalr||rob.front().cmd.type==jal){
            /*clear*/
            if(rob.front().cmd.tpe==B)
                cnt++;
            while(!queue.empty()){
                queue.pop();
            }
            for(int i=1;i<=64;++i){
                rs[i]=reorder();
            }
            for(int i=0;i<=31;++i){
                reg.reorder[i]=0;
            }
            while(!rob.empty()){
                rob.pop();
            }
            rs_num=sl_num=0;
            reg.data[0]=0;
            //cout<<PC<<' '<<8888888888<<std::endl;
            return 0;
        }else rob.pop();//Not Taken{

    }
    //  cout<<PC<<std::endl;
    // printreg();
    // printrs();
    // printrob();
    // cout<<std::endl;
    if(stuck==0)PC+=4;
    return 0;
}



int main() {
    int tmp;
    mem.get_all();
    while(1){
        tmp=run();
        if(tmp){
            cout<<tmp<<std::endl;
            break;
        }

        cycle++;
        //if(cycle==9999)break;
    }



    //printInt(177);
    return 0; // 94
}
