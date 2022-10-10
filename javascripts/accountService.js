
const AccountService = {

    /**
     * 
     */
    register(player, username, password) {
        print(username);
        let findUser = db.account.find({ 'username' : username });
        if (findUser) {
            // username repeated.
            return 1;
        }
        
        const salt = randomString(4);
        const pwd = sha256(salt + password);
        let user = new domain.Account();
        user.username = username;
        user.password = pwd;
        user.salt = salt;
        user.createTime = currentTime();
        user.lastLoginTime = currentTime();
        user._id = db.account.insert(user);
        return 0;
    },

    /**
     * 
     * @param {*} player 
     * @param {*} username 
     * @param {*} password 
     * @returns 
     */
    login(player, username, password) {
        let findUser = db.account.find({ 'username' : username });
        if(!findUser) {
            return 1;
        }

        let pwd = sha256(findUser.salt + password);
        if(pwd != findUser.password) {
            return 2;     // password error
        }

        print(`account ${username}, ${findUser._id} is login.`);
        
        return 0;
    },
    
};

// let user = AccountService.register({}, 'a123456', '123456');
// if (typeof user === 'object') {
//     print("after register, user: " + JSON.stringify(toJsObject(user)));    
// } else {
//     print("after register, result: " + user);
// }


let tmp = AccountService.login({}, 'a123456', '123456');
print('login result: ' + JSON.stringify(toJsObject(tmp)));

// Services.AccountService = AccountService;
registerServiceObject('AccountService', AccountService);
