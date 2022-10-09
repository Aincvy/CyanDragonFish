// second load .

print(typeof domain.Account);
db.account.entityCtor = domain.Account;
print(typeof db.account.entityCtor);

let a = new domain.Account();
a.username = "111";
a.password = "222";
a.salt = "--xxx---";
a.lastLoginTime = currentTime() - 1000;
// print(a);
// print(JSON.stringify(a));
// print(JSON.stringify(domain.Account));
// print(Object.getOwnPropertyNames(a));

let n = {};
for(let k in a) {
    // print(k);
    // print(a[k]);
    n[k] = a[k];
}
print(JSON.stringify(n));

print(typeof sha256);
print(sha256('x1x2x3'))

// db.account.insert(a);
let result = db.account.find({'username': a.username});
print(JSON.stringify(result));
n = {};
for(let k in result) {
    n[k] = result[k];
}
print(JSON.stringify(n));
