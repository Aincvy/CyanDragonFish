
const AccountService = {

    /**
     * 
     */
    register(player, username, password) {
        let findUser = db.account.find({ 'username' : username });
        if (findUser) {
            // username repeated.
            return 1;
        }
        
        const salt = randomString();
        const pwd = sha256(salt + password);
        let user = new domain.Account();
        user.username = username;
        user.password = pwd;
        user.salt = salt;
        user.createTime = currentTime();
        user.lastLoginTime = currentTime();
        user._id = user.id = db.account.insert(user);
        return user;
    },

    login(player, username, password) {
        let findUser = db.account.find({ 'username' : username });
        if(!findUser) {
            return 1;
        }

        let pwd = sha256(findUser.salt + password);
        if(pwd != findUser.password) {
            return 2;     // password error
        }

        print(`account ${username}, ${findUser.id} is login.`);
        return findUser;
    },
    
};

