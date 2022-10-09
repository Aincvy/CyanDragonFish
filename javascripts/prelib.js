// first load file.

// database part.
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

    function mkObject(from, entityCtor) {
        from._id = from._id.$oid;
        let entity = entityCtor();
        copyObject(from, entity);
        return entity;
    }

    if(typeof r === 'object' ) {
        if (r.constructor.name == "Array") {
            // multiple object
            let array = Array(r.length);
            for(let i in r) {
                array.push(mkObject(i, this.entityCtor));
            }
            return array;
        } else {
            // single object
            return mkObject(r, this.entityCtor);
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

// util function .

function sha256(obj) {
    return sha256_js.sha256(obj);
}

function toJsObject(nativeObj) {
    let n = {};
    for(let k in nativeObj) {
        n[k] = nativeObj[k];
    }
    return n;
}

//  service part 
const Services = {};

function registerServiceObject(name,  obj) {
    if (typeof obj !==  'object') {
        return;
    }

    print('Register service object: ' + name);
    Services[name] = obj;

    for(let k  in obj) {
        let n = name ? `${name}.${k}` : k;
        let f = obj[k];
        if (typeof f === 'function') {
            registerService(n, f);    
        }
    }
}

