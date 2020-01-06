# How to calculate ip-networks using bit-operators in C

Sometimes we have to calculate networks. We are interested in stuff like: networkid, subnetmask, min. hostip, max. hostip, how many hosts can we address in this network aso.. There are many tools for calculating networks. And they are perfectly good. If you need it in a c programm, there are libraries for that too. But I think it's not neccessary to use libraries for that. If you calculate it manually, it's just a matter of working with bits in a 32bit address, so it might be an quite easy task... 

I wrote this little programm to demonstrate how it works

## Compile

```
gcc -o calciprog calciprog.c
```

## Example

```
doctor@tardis> ./calciprog 192.168.10.0 255.255.255.0
Network:        192.168.10.0/24
Wildcard:       0.0.0.255
Netmask:        255.255.255.0
Hostmin:        192.168.10.1
Hostmax:        192.168.10.254
Broadcast:      192.168.10.255
Hosts:          254
NetID-Bytewise: 0 10 168 192 -> 698560
```

## Documentation

https://tech.feedyourhead.at/content/how-calculate-ip-networks-using-bit-operators-c

## License

GPL

## Author Information

Wolfgang Hotwagner (https://tech.feedyourhead.at/)
