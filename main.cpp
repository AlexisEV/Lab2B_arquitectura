#include <iostream>
#include <bitset>
#include <tuple>

std::tuple<std::bitset<1>, std::bitset<8>, std::bitset<32>> floatABinario(float numero) {
    // float a binario
    std::bitset<32> binario(*reinterpret_cast<unsigned int*>(&numero));
    std::cout << "\n\nNumero original: " << numero;
    std::cout << "\nNumero original binario: " << binario;

    // separar signo, exponente y significando
    std::bitset<1> signo(binario[31]);
    std::bitset<8> exponente;
    for (int i = 0; i < 8; ++i) {
        exponente[7-i] = binario[30 - i];
    }
    std::cout <<"\nexponente:" << exponente;

    std::bitset<32> significandoExtendido;
    significandoExtendido[31] = 1; // Bit implicito
    for (int i = 0; i < 23; ++i) {
      significandoExtendido[30 - i] = binario[22 - i];
    }
    std::cout <<"\nsignificando:" << significandoExtendido;
    //  bits restantes de significandoExtendido son 0

    return {signo, exponente, significandoExtendido};
}

float binarioAFloat(std::bitset<32> binario) {
    unsigned int num = binario.to_ulong();
    return *reinterpret_cast<float*>(&num);
}

std::bitset<1> determinarSigno(std::bitset<1> x, std::bitset<1> y){
  return x^y;
}

bool detectaOverflowExponente(std::bitset<8> exponente) {
    return exponente.to_ulong() > 254;
}

bool detectaUnderflowExponente(std::bitset<8> exponenteSumado) {
    return exponenteSumado.to_ulong() == 0;
}


std::bitset<8> sumaExponentes(std::bitset<8> a, std::bitset<8> b) {
  int bias = 127;
  int exp_a = a.to_ulong();
  int exp_b = b.to_ulong();
  int suma = exp_a + exp_b - bias;
  std::bitset<8>resultado (suma);
  std::cout << "\n\nSUMA DE EXPONENTES: " << resultado;
  return resultado;
}

std::tuple<std::bitset<23>, int> normalizaYRedondea(std::bitset<64> significandoMult) {
    int punto = 62; //El punto siempre estará en la posición 62
    int desplazamiento = 0;
    
    // identificar la primera posicion con 1
    int primeraPos = -1;
    for (int i = 63; i >= 0; --i) {
        if (significandoMult[i] == 1) {
            primeraPos = i;
            break;
        }
    }

    // Si el número es 0
    if (primeraPos == -1) {
        return {std::bitset<23>(0), 0};
    }
    
    // calcular el desplazamiento
    desplazamiento = primeraPos-punto;
    //std::cout << "\nPunto, desplazamiento, primeraPos: " << punto << ", " << desplazamiento << ", " << primeraPos; 

    // extrayndo los 24 bits más significativos
    std::bitset<23> significandoNormalizado(0);
    for (int i = 0; i < 23; ++i) {
      significandoNormalizado[i] = significandoMult[i + (primeraPos-23) + 1];
    }
    std::cout << "\n\nSignificando recortado: " << significandoNormalizado;

    // Redondeo
    if (significandoMult[21 + desplazamiento] == 1) {
        significandoNormalizado = std::bitset<23>(significandoNormalizado.to_ulong() + 1);
    }
    std::cout << "\nSignificando normalizado: " << significandoNormalizado;
    std::cout << "\nDesplazamiento exponente: " << desplazamiento;

    return {significandoNormalizado, desplazamiento};
}


std::bitset<32> sumaBinaria(std::bitset<32> a, std::bitset<32> b, std::bitset<1>& c) {
  std::bitset<32> resultado;
  int acarreo = 0;
  for (int i = 0; i < 32; i++) {
    int suma = a[i] + b[i] + acarreo;
    resultado[i] = suma % 2;
    acarreo = suma / 2;
  }
  c[0] = acarreo;
  return resultado;
}

std::bitset<64> multiplicaSignificandosBits(std::bitset<32> a, std::bitset<32> b) {
  std::bitset<1> C(0);
  std::bitset<32> A(0);
  std::bitset<32> Q = a;
  std::bitset<32> M = b;
  std::bitset<64> resultado;

  for(int i = 32; i>0; i--){
    if(Q[0] == 1){
      A = sumaBinaria(A,M,C);
    }
    Q = Q >> 1;
    Q[31] = A[0];
    A = A >> 1; 
    A[31] = C[0];
    C = C >> 1;
  }

  // copiar bits de A
  for(int i = 0; i < 32; ++i) {
      resultado[i] = Q[i];
  }
  // copiar bits de Q
  for(int i = 0; i < 32; ++i) {
      resultado[i + 32] = A[i];
  }
  std::cout << "\nMultiplicacion significando: " << resultado;
  return resultado;
}


std::tuple<std::bitset<23>, int> multiplicaYNormaliza(std::bitset<32> a, std::bitset<32> b) {
    std::bitset<64> multiplicacion = multiplicaSignificandosBits(a, b);
    return normalizaYRedondea(multiplicacion);
}

void ajustaExponente(std::bitset<8>& exponente, int desplazamiento) {
    int valorExponente = static_cast<int>(exponente.to_ulong());
    valorExponente += desplazamiento;
    
    // posibles underflows y overflows
    if(valorExponente > 254) {
        std::cout << "Overflow de exponente detectado tras ajuste, operación finalizada";
        exit(1);
    } else if(valorExponente <= 0) {
        std::cout << "Underflow de exponente detectado tras ajuste, operación finalizada";
        exit(1);
    }

    exponente = std::bitset<8>(valorExponente);
}

std::bitset<32> unirFloat(std::bitset<1> signo, std::bitset<8> exponente, std::bitset<23> significando) {
    std::bitset<32> resultado;
  
    //bit de signo
    resultado[31] = signo[0];

    //bits del exponente
    for (int i = 0; i < 8; ++i) {
        resultado[30 - i] = exponente[7 - i];
    }
  
    significando = significando << 1;
    //std::cout << "\nSignificando sin bit implicito: " << significando;

    //bits del significando
    for (int i = 0; i < 23; ++i) {
        resultado[22 - i] = significando[22 - i];
    }

    return resultado;
}

std::bitset<32> mult_float(float x, float y){
  //si uno de los dos es 0, el resultado es 0
  if(x == 0|| y == 0){
    std::bitset<32> resultado(0);
    return resultado;
  }
  std::bitset<1> signo_x, signo_y, signo_z;
  std::bitset<8> exponente_x, exponente_y, exponente_z;
  std::bitset<32> significando_x, significando_y;
  std::bitset<23> significando_z;
  int desplazamientoExp = 0;
  int posPunto = 0;
  
  signo_z = determinarSigno(signo_x, signo_y);
  
  //transformacion a binario y extraccion de signo, exponente y significando
  std::tie(signo_x, exponente_x, significando_x) = floatABinario(x);
  std::tie(signo_y, exponente_y, significando_y) = floatABinario(y);

  //suma de exponentes
  exponente_z = sumaExponentes(exponente_x, exponente_y);

  //deteccion de overflows
  if(detectaOverflowExponente(exponente_z)){
    std::cout << "Overflow de exponente detectado, operación finalizada";
    exit(1);
  }
  if(detectaUnderflowExponente(exponente_z)){
    std::cout << "Underflow de exponente detectado, operación finalizada";
    exit(1);
  }

  //multiplicacion de significantes, normalizacion y redondeo
  std::tie(significando_z, desplazamientoExp) = multiplicaYNormaliza(significando_x, significando_y);
  ajustaExponente(exponente_z, desplazamientoExp);
  
  //ensamblado del float de 32 bits
  return unirFloat(signo_z, exponente_z, significando_z);
}

int main() {
  float x;
  float y;
  std::bitset<32> resultado;

  std::cout << "Introduce dos números float para multiplicar: ";
  std::cin >> x >> y;

  resultado = mult_float(x, y);

  float c = x*y;
  std::bitset<32> binario(*reinterpret_cast<unsigned int*>(&c));
  
  std::cout << "\n\nResultado en binario: " << resultado ;
  std::cout << "\nResultado en float: " << binarioAFloat(resultado);
  std::cout << "\nResultado en C++ binario: " << binario;
  std::cout << "\nResultado en C++: " << x*y;
  

  return 0;
}