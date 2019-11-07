//Hola Silver, voy a ir empezando.
var total= new Array;
var sum;
var i=0;


function calculaTarifa(tarifaPorZona, masZ, menosZ, parada, persona, coche, distancia){
    var res;

    res=tarifaPorZona(tarifaPorZona, masZ, menosZ);
    
    if(!coche){
        res*=1.2;
    }
    if(parada){
        sum+=15;
    }

    if(persona){
        i+=1;
    }

    res*=distancia;
    res+=sum;

    if(i>0){
        var j;
        for(j=0; j<=i; j++){
            total[j]+=res;
        }
    }else{
        total[0]=res;
    }
    sum=0;

}

//la tarifa de las zonas van cambiando +2
function tarifaPorZona(anterior, mas, menos){
    var res;

    if(anterior>0){
        if(mas.equals(menos)){
            res=anterior;
        }
        else{
            if(mas){
                res=anterior+2;
            }else{
                res=anterior-2;
            }
        } 

    } else {
        res=5;
    }       
    return res;
}

