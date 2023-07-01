
### 数据结构
基类：`LSA`
```c++
struct LSA {
    LSAHeader lsa_header;

    LSA();
    virtual char* toLSAPacket() = 0;
    virtual size_t size() = 0;
    bool operator>(const LSA& other);
};
``

派生类：`LSARouter`，`LSANetwork`
```c++
struct LSARouter : public LSA {
    // LSAHeader   lsa_header;
    /* data part */
    uint8_t     zero1 : 5;
        uint8_t     b_V : 1;    // Virtual : is virtual channel
        uint8_t     b_E : 1;    // External: is ASBR
        uint8_t     b_B : 1;    // Board   : is ABR
    uint8_t     zero2 = 0;
    uint16_t    link_num;
    std::vector<LSARouterLink> links;

    LSARouter();
    LSARouter(char* net_ptr);
    char* toLSAPacket() override; // [attention]: need to be del
    char* toRouterLSA();  // [attention]: need to be del, TODO change to toLSAPacket()
    size_t size() override;
    bool operator==(const LSARouter& other);
    // bool operator>(const LSARouter& other);
};

struct LSANetwork : public LSA {
    // LSAHeader   lsa_header;
    /* data part */
    uint32_t    network_mask;   // init
    std::vector<uint32_t> attached_routers; // assign: ip list

    LSANetwork();
    LSANetwork(char* net_ptr);
    char* toLSAPacket() override; // [attention]: need to be del
    char* toNetworkLSA(); // [attention]: need to be del, TODO change to toLSAPacket()
    size_t size() override;
    bool operator==(const LSANetwork& other);
};
```

### 注意事项
- 由于使用了虚函数（纯虚函数），所以不管是基类还是派生类实例在内存中头部均为**虚函数表**，而不是`LSAHeader`，所以对于`char*`指向的连续内存块（报文段），不再能用类似`LSARouter*`直接指向并解析，而得用`LSAHeader*`去指向解析
- TODO：将`toRouterLSA()`等原先未是实现多态时携带代码统一成多态后的简洁形式！