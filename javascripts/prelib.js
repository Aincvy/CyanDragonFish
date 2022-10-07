// first load .
database.collection.prototype.find = function(condition = {}) {
    let r = this._find(condition);
    if(!r) {
        return r;
    }

    function copyObject(src, dst) {
        for(let k in src) {
            dst[k] = src[k];
        }
    }

    function mkObject(from) {
        from._id = from._id.$oid;
        let entity = this.entityCtor();
        copyObject(from, entity);
        return entity;
    }

    if(typeof r === 'object' ) {
        if (r.constructor.name == "Array") {
            // multiple object
            let array = Array(r.length);
            for(let i in r) {
                // let entity = this.entityCtor();
                // copyObject(i, entity);
                array.push(mkObject(i));
            }
            return array;
        } else {
            // single object
            return mkObject(r);
        }
    } else {
        print('a inner error is happened..');
        return undefined;
    }
}

database.collection.prototype.insert = function(value) {
    if(!value) {
        return ;
    }
    if(typeof value !== 'object') {
        print('insert function only support object type');
        return;
    }
    if(typeof value.createTime !== 'number') {
        print("value.createTime is not long, change it.");
        value.createTime = currentTime();
    } else if (value.createTime === 0) {
        value.createTime = currentTime();
    }
    value.updateTime = currentTime();

    let n = {};
    for(let k in value) {
        if(k == '_id') {
            continue;
        }
        n[k] = value[k];
    }
    return this._insert(n);
}

