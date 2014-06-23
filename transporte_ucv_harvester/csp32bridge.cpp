/*******************************************************************
$Header: C:/Symbol/Csp32Proj/Csp32/src/vcs/Csp32.cpv  1.0  05 Mar 1999  10:00:00 kFallon $
 *
 * © Copyright 1999 Peninsula Solutions, Inc., all rights reserved.
 * © Copyright 1999 Symbol Technologies, Inc., all rights reserved.
 *
 *          ***************************************
 *          * This file was generated by:         *
 *          *       Peninsula Solutions, Inc.     *
 *          *       1265 S. Semoran Blvd.         *
 *          *       Suite 1247                    *
 *          *       Winter Park, FL 32792         *
 *          *       Phone: (407) 673-6544         *
 *          *       Fax:   (407) 673-6545         *
 *          * E-mail: info@PeninsulaSolutions.com *
 *          *                                     *
 *          * under a grant from:                 *
 *          *       Symbol Technologies, Inc.     *
 *          *       Holtsville, NY                *
 *          ***************************************
 *
 * The user is granted a license to use and modify this file for their own
 * purposes so long as the Copyright information remains intact.
 *
 *    MODULE NAME:   Csp32.cpp
 *
 *    DESCRIPTION:
 *                   This file provides the user API interface
 *                   functions for Symbol's Consumer Scanning
 *                   Products as Dynamic Link Library. When
 *                   compiled into a DLL, the user can access
 *                   all of the functions available for the
 *                   Symbol CyberPen.
 *
 *    AUTHOR:        Kim Fallon
 *
 *    DATE CREATED:  03/05/99
 *
 *    HISTORY:
 *
 *       03/05/99    Initial Development
 *
$Log: C:/Symbol/Csp32Proj/Csp32/src/vcs/Csp32.cpv $
 *******************************************************************/
/*******************************************************************
 * MODIFICACION HECHA POR PERSONAL DE COMPUSCIENS
 *
 * TODO EL TRABAJO FUE DESARROLLADO ORIGINALMENTE POR EL O LOS PARTICIPANTES
 * ESPECIFICADOS EN LA NOTA SUPERIOR A ESTA.
 *
 * PROYECTO TRANSPORTE UCV - 19/06/2014
 *
 ******************************************************************/


#include "csp32bridge.h"

Csp32Bridge::Csp32Bridge(QObject *parent) : QObject(parent)
{
    nCspActivePort= -1; // Ningun puerto seleccionado
    nCspDeviceStatus= -1; // No hay un estado actual
    nCspProtocolVersion= -1; // No hay version del protocolo disponible
    nCspSystemStatus= -1; // No hay estado del sistema disponible

    nCspStoredBarcodes= 0; // Numeros de codigos de Barras almacenados
    nCspRetryCount= 5; // Numero por default de interrogaciones al dispositivo
}

/** Y ACA EMPIEZAN TODOS LOS METODOS NECESARIOS PARA LA COMUNICACION CON EL LECTOR **/
/** **/
/** **/

/* Esta funcion lee un caracter del puerto serial activo, si el puerto serial
 * no tiene un caracter dentro de MAXTIME, la funcion retorna un error de comunicacion
 *
 * Retorna el caracter leido o COMMUNICATIONS_ERROR
 */
int Csp32Bridge::cspGetc()
{
    int i;
    int  chRead;

    // Aseguremonos que se ha escogido un puerto
    if (nCspActivePort >= COM1)
    {
        // Podemos leer datos de el?
        for (i = 0; i < MAXTIME; i++)
        {
            // Tratemos de leer un caracter
            if ((chRead = SioGetc(nCspActivePort)) != WSC_NO_DATA)
            {
                return((int) chRead);
            }

            // Dejemos que otras tareas se ejecuten mientras esperamos por el lector
            Sleep(50);
        }
    }

    // Si llegamos hasta aca, ningun caracter pudo leerse, asi que...
    return (COMMUNICATIONS_ERROR);
}

/* Lrc es un metodo de deteccion de errores, esta funcion se encarga de
 * calcular el codigo LRC y retornarlo */
char Csp32Bridge::cspLrcCheck(char aLrcBytes[], int nMaxLength)
{
    int i;
    unsigned char chLrc = 0;        // inicializemos el valor del LRC

    // Calculemos el LRC del string completo
    for (i = 0; i < nMaxLength; i++)
        chLrc ^= aLrcBytes[i];

    return(chLrc);
}

/* Invalida o inicializa todos los valores usados por la clases
 * Probablemente usada despues de la configuracion de un nuevo puerto
 * COM
 */
void Csp32Bridge::cspInitParms()
{
    nCspActivePort= -1; // Ningun puerto seleccionado
    nCspDeviceStatus= -1; // No hay un estado actual
    nCspProtocolVersion= -1; // No hay version del protocolo disponible
    nCspSystemStatus= -1; // No hay estado del sistema disponible

    nCspStoredBarcodes= 0; // Numeros de codigos de Barras almacenados
}

/* Esta funcion maneja el overhead asociado al envio de comandos al dispositivo
 * CSP (Interrogation, Read Barcodes, Clear Barcodes, etc). */
int Csp32Bridge::cspSendCommand (char *aCommand, int nMaxLength)
{
    int i;

    // Aseguremonos que tenemos un puerto seleccionado valido
    if (nCspActivePort < COM1)
        return( COMMUNICATIONS_ERROR );

    // Un tiempo antes de enviar el mensaje
    Sleep ( 120 );

    // Limpiemos la cola de recepcion
    while (SioGetc(nCspActivePort) >= 0);

    // Enviemos el string con el comando
    for ( i = 0; i < nMaxLength; i++)
    {
        SioPutc(nCspActivePort, aCommand[i]);
    }

    // Esperemos la respuesta
    nCspDeviceStatus  = cspGetc();
    aByteBuffer[0] = (char) nCspDeviceStatus;

    // Conversion del estado del dispositivo a los estados definidos en el .h
    nCspDeviceStatus *= -1;

    // Si no hay una respuesta, retornemos un error
    if ( nCspDeviceStatus > 0 )
        return( COMMUNICATIONS_ERROR );

    if ( nCspDeviceStatus != NO_ERROR_ENCOUNTERED )
        return ( nCspDeviceStatus );
    else
        return ( STATUS_OK );
}


// FUNCIONES DE COMUNICACION

/* Esta funcion abre el puerto de comunicaciones especificado por
 * nComPort. Ademas de esto, inicializa todas las estructuras requeridas
 * para el funcionamiento de la comunicacion con el lector. */

int Csp32Bridge::cspInit(int nComPort)
{
    // El usuario trata de abrir un puerto valido?
    if (nComPort >= COM1)
    {
        // Cierra cualquier otro puerto abierto previamente
        if (nCspActivePort >= COM1)
            cspRestore();

        // Ahora tratamos de inicializar el puerto escogido
        if (SioReset(nComPort, RX_QUE_SIZE, TX_QUE_SIZE) < 0)
        {
            // Esto ocurrira si ningun puerto esta activo
            return(nCspActivePort = COMMUNICATIONS_ERROR);
        }
        else
        {
            // En caso contrario, el nuevo puerto activo sera el escogido...
            nCspActivePort = nComPort;
        }

        // Establece los parametros del puerto para asegurar la compatibilidad con el lector
        SioParms(nCspActivePort, OddParity, OneStopBit, WordLength8);
        SioBaud(nCspActivePort, Baud2400);
        SioDTR(nCspActivePort, SET_LINE);
        SioRTS(nCspActivePort, SET_LINE);
        return (STATUS_OK);
    }

    return (BAD_PARAM);
}


/* Esta funcion trata de cerrar cualquier conexion anteriormente abierta */
int Csp32Bridge::cspRestore()
{
    int nRetStatus = COMMUNICATIONS_ERROR;

    // are we attempting to close a valid port?
    if (nCspActivePort >= COM1)
    {
        // close any previously opened ports...
        nRetStatus = SioDone(nCspActivePort);
    }

    // initialize the dll interface...
    cspInitParms();

    // return status...
    if (nRetStatus < 0)
        return (COMMUNICATIONS_ERROR);
    else
        return (STATUS_OK);
}

// FUNCIONES BASICAS

/* Esta funcion se encarga de leer los codigos escaneados, el id del dispositivo y la firma
 * del mismo. */
int Csp32Bridge::cspReadData()
{
    int nRetStatus;

    // read the data from the CSP device
    nRetStatus = cspReadRawData(NULL, DETERMINE_SIZE);

    // if response not available, reply with error
    if ( nRetStatus < 0 )
        return ( nRetStatus );

    return( nCspStoredBarcodes );
}

/* Esta funcion limpia todos los codigos guardados en el lector */
char aClearBarCodesCmd[] =
{
    CLEAR_BAR_CODES,         // Comando de limpieza de codigos
    STX,                     // Condigo de opcion
    0x00,                    // Caracter nulo para cerrar el mensaje
    0x00                     // LRC
};

int Csp32Bridge::cspClearBarCodes()
{
    int i;
    int nRetStatus;

    // Revisemos que el lector este conectado...
    if ((nRetStatus = cspInterrogate()) != STATUS_OK)
        return( nRetStatus );

    // Enviemos el comando al lector.s..
    nRetStatus = cspSendCommand (aClearBarCodesCmd, sizeof(aClearBarCodesCmd));

    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // Hasta aqui todo bien, obtengamos el mensaje entero...
    i = 1;

    // Obtengamos el caracter de opcion (STX)
    aByteBuffer[i++] = (char) cspGetc();

    // Obtengamos el caracter nulo
    aByteBuffer[i++] = (char) cspGetc();

    // Verifiquemos el LRC...
    aByteBuffer[i] = cspLrcCheck(aByteBuffer, i);
    if ( aByteBuffer[i] != (char) cspGetc())
        return( COMMAND_LRC_ERROR );

    return( STATUS_OK );
}

/* Esta funcion ordena al dispositivo a que cierre la comunicacion y entre en
 * modo de parada hasta que algun otro evento vuelva a ocurrir. */
char aPowerDownCmd[] =
{
    POWER_DOWN,              // Comando de Apagado
    STX,                     // Codigo de opcion
    0x00,                    // Caracter nulo para cerrar el mensaje
    0x07                     // LRC
};

int Csp32Bridge::cspPowerDown()
{
    int i;
    int nRetStatus;

    // Envia el comando al dispositivo CSP (lector)
    nRetStatus = cspSendCommand (aPowerDownCmd, sizeof(aPowerDownCmd));

    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // Hasta aca todo bien, ahora a obtener la respuesta completa
    i = 1;

    // Leamos el codigo de opcion (STX)
    aByteBuffer[i++] = (char) cspGetc();

    // Leamos el caracter nulo
    aByteBuffer[i++] = (char) cspGetc();

    // Verifiquemos el LRC...
    aByteBuffer[i] = cspLrcCheck( aByteBuffer, i );
    if ( aByteBuffer[i] != (char) cspGetc())
        return( COMMAND_LRC_ERROR );

    return( STATUS_OK );
}

// FUNCIONES CSP DATA GET


/* Esta funcion copia los datos del codigo de barras de la estructura szCspBcString[]
 * a szBarData[], truncando a los nMaxLength caracteres */
int Csp32Bridge::cspGetBarcode(char szBarData[], int nBarcodeNumber, int nMaxLength)
{
    int i;
    int nBarFound;
    int nBarLength;

    // Aseguremonos que el codigo solicitado es valido
    if (nBarcodeNumber < 0)
        return( BAD_PARAM );

    if (nBarcodeNumber >= nCspStoredBarcodes)
        return( BAD_PARAM );

    // Todo ha ido bien, asi que bamos a obtener el codigo
    i = 0;
    nBarFound = 0;
    while (nBarFound < nBarcodeNumber)
    {
        // Busquemos el caracter nulo que separa los strings
        while (szCspBarData[i++] != 0);

        // Saltemos al siguiente codigo de barras
        nBarFound++;
    }

    // Ahora que tenemos el codigo, toca procesarlo
    nBarLength = strlen(&szCspBarData[i]) + 1;

    // El usuario solo pidio el tamaño del string?
    if (nMaxLength > DETERMINE_SIZE)
    {
        // Obtengamos el numero maximo de caracteres a copiar
        nMaxLength = min(nBarLength,nMaxLength);

        // Copiemos el codigo de barras
        memcpy(szBarData, &szCspBarData[i], nMaxLength);

        // Y luego el caracter nulo para cerrar el string
        szBarData[nMaxLength-1] = 0;
    }

    return(nBarLength);
}

// Esta funcion devuelve el ID del dispositivo almacenado en la clase
int Csp32Bridge::cspGetDeviceId(char szDeviceId[9], int nMaxLength)
{
    // El usuario solo pidio el tamaño del string?
    if (nMaxLength > DETERMINE_SIZE)
    {
        // Obtengamos el numero maximo de caracteres a copiar
        nMaxLength = min((int)sizeof(szCspDeviceId),nMaxLength);

        // Copiemos el ID del dispositivo
        memcpy(szDeviceId, szCspDeviceId, nMaxLength);

        // Y luego el caracter nulo para cerrar el string
        szDeviceId[nMaxLength-1] = 0;
    }

    return( sizeof(szCspDeviceId) );
}

/* Esta funcion devuelve el string de la firma del dispositivo almacenada
 * en la clase
 */
int Csp32Bridge::cspGetSignature(char aSignature[8], int nMaxLength)
{
    // El usuario solo pidio el tamaño del string?
    if (nMaxLength > DETERMINE_SIZE)
    {
        // Obtengamos el numero maximo de caracteres a copiar
        nMaxLength = min((int)sizeof(aCspSignature),nMaxLength);

        // Copiemos la firma del dispositivo
        memcpy(aSignature, aCspSignature, nMaxLength);
    }

    return( sizeof(aCspSignature) );
}

/* Esta funcion devuelve un entero que corresponde al identificador del protocolo
 * usado por el dispositivo */
int Csp32Bridge::cspGetProtocol()
{
    return (nCspProtocolVersion);
}

/* Esta funcion retorna la version de firmware (?) del dispositivo CSP guardada
 * en la clase. El string de salida termina en un caracter nulo*/
int Csp32Bridge::cspGetSwVersion(char szSwVersion[9], int nMaxLength)
{
    // El usuario solo pidio el tamaño del string?
    if (nMaxLength > DETERMINE_SIZE)
    {
        // Obtengamos el numero maximo de caracteres a copiar
        nMaxLength = min((int)sizeof(szCspSwVersion),nMaxLength);

        // Copiemos la version de software del dispositivo
        memcpy(szSwVersion, szCspSwVersion, nMaxLength);

        // Y luego el caracter nulo para cerrar el string
        szSwVersion[nMaxLength-1] = 0;
    }

    return( sizeof(szCspSwVersion) );
}

// FUNCIONES SET DE LA CONFIGURACION DEL DISPOSITIVO

/* Esta funcion guarda 8 Bytes de datos especificos del host en el lector*/
int Csp32Bridge::cspSetTlBits(char aTlBits[8], int nMaxLength)
{
    // El usuario ha proveido suficientes bits TL?
    if (nMaxLength < 8)
        return( BAD_PARAM );

    // Establezcamos los bits TL
    return( cspSetParam( TL_BITS, aTlBits, nMaxLength ) );
}

/* Esta funcion es para configurar el volumen del BEEP que emite el lector */
int Csp32Bridge::cspSetVolume(int nVolume)
{
    char aVolume[2];

    // El volumen solicitado es valido?
    if ((nVolume < VOLUME_QUIET) || (nVolume > VOLUME_HIGH))
        return( BAD_PARAM );

    // Si es asi, configuremos el volumen nuevo
    aVolume[0] = (char) nVolume;
    return( cspSetParam( VOLUME, aVolume, 1 ) );
}

/* Esta funcion alterna la presencia de redundancia de codigos de barra del dispositivo */
int Csp32Bridge::cspSetBarcodeRedundancy(int nOnOff)
{
    char aBcRedundancy[2];

    // La configuracion de redundancia es valida?
    if ((nOnOff < PARAM_OFF) || (nOnOff > PARAM_ON))
        return( BAD_PARAM );

    // Si es asi, vamos a configurarla de acuerdo a nOnOff
    aBcRedundancy[0] = (char) nOnOff;
    return( cspSetParam( BARCODE_REDUNDANCY, aBcRedundancy, 1 ) );
}

/* Esta funcion escribe el string del ID de usuario en el dispositivo */
int Csp32Bridge::cspSetUserId(char szUserId[9], int nMaxLength)
{
    // El usuario suministro suficientes datos?
    if (nMaxLength < 8)
        return( BAD_PARAM );

    // Configuremos el ID de usuario
    return( cspSetParam( USER_ID, szUserId, 8 ) );
}

/* Esta funcion alterna la presencia del escaneo continuo del lector */
int Csp32Bridge::cspSetContinuousScanning(int nOnOff)
{
    char    aContScanning[2];

    // La configuracion de redundancia es valida?
    if ((nOnOff < PARAM_OFF) || (nOnOff > PARAM_ON))
        return( BAD_PARAM );

    // Configuremos el escaneo continuo de acuerdo a nOnOff
    aContScanning[0] = (char) nOnOff;
    return( cspSetParam( CONTINUOUS_SCANNING, aContScanning, 1 ) );
}


// FUNCIONES GET DE LA CONFIGURACION DEL DISPOSITIVO

/* Esta funcion retorna los bits TL del dispositivo */
int Csp32Bridge::cspGetTlBits(char aTlBits[8], int nMaxLength)
{
    int nRetStatus;

    // Obtengamos los bits TL del dispositivo
    nRetStatus = cspGetParam(TL_BITS, aCspTlBits, sizeof(aCspTlBits));

    // Si no hay respuesta, retornemos un error
    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // El usuario solo pidio la intitud del string?
    if (nMaxLength > DETERMINE_SIZE)
    {
        // Obtengamos el numero maximo de caracteres a copiar
        nMaxLength = min((int)sizeof(aCspTlBits),nMaxLength);

        // Copiemos los bits TL del lector
        memcpy(aTlBits, aCspTlBits, nMaxLength);
    }

    return( sizeof(aCspTlBits) );
}

/* Esta funcion retorna la configuracion de volumen del dispositivo */
int Csp32Bridge::cspGetVolume()
{
    int nRetStatus;
    char aVolume[2];


    // Pidamos la informacion del volumen
    nRetStatus = cspGetParam(VOLUME, aVolume, 1);

    // Si no hay respuesta, retornamos un error
    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // Retornamos la configuracion actual de volumen
    nCspVolume = (int) aVolume[0];

    return( nCspVolume );
}

/* Esta funcion retorna la configuracion de redundancia de codigos del dispositivo */
int Csp32Bridge::cspGetBarcodeRedundancy()
{
    int nRetStatus;
    char aBcRedundancy[2];

    // Solicita la configuracion de redundancia del lector
    nRetStatus = cspGetParam(BARCODE_REDUNDANCY, aBcRedundancy, 1);

    // Si no hay respuesta, retornamos un error
    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // Retornamos la configuracion de redundancia actual
    nCspBarcodeRedundancy = (int) aBcRedundancy[0];
    return( nCspBarcodeRedundancy );
}

/* Esta funcion retorna el ID de usuario guardada en la clase, un string terminado en NULL */
int Csp32Bridge::cspGetUserID(char szUserId[9], int nMaxLength)
{
    int nRetStatus;

    // Ahora, obtengamos el USER ID del dispositivo
    nRetStatus = cspGetParam(USER_ID, szCspUserId, sizeof(szCspUserId) - 1);

    // Si no hay respuesta, retornamos un error
    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // El usuario solo pidio la intitud del string?
    if (nMaxLength > DETERMINE_SIZE)
    {
        // Obtengamos el numero maximo de caracteres a copiar
        nMaxLength = min((int)sizeof(szCspUserId),nMaxLength);

        // Copiamos el ID de usuario
        memcpy(szUserId, szCspUserId, nMaxLength);

        // Y le pegamos el caracter nulo al final
        szUserId[nMaxLength-1] = 0;
    }

    return( sizeof(szCspUserId) );
}

/* Esta funcion retorna la configuracion actual de escaneo continuo del dispositivo */
int Csp32Bridge::cspGetContinuousScanning()
{
    int nRetStatus;
    char aContScanning[2];

    // Ahora, obtengamos la configuracion de escaneo continuo
    nRetStatus = cspGetParam(CONTINUOUS_SCANNING, aContScanning, 1);

    // Si no hubo respuesta, retornemos un error
    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // Retornemos la configuracion de escaneo continuo leida
    nCspContinuousScanning = (int) aContScanning[0];
    return( nCspContinuousScanning );
}


// FUNCIONES DE CONFIGURACION DE LA CLASE

/* Esta funcion configura el numero maximo de reintentos de conexion que se
 * ejecutan sobre el dispositivo, dandole al usuario un tiempo razonable
 * para conectarlo o corregir cualquier error contextual. El default es 5 */
int Csp32Bridge::cspSetRetryCount(int nRetryCount)
{
    // Aseguremonos que el numero de reintentos esta entre 0 y 9
    if ((nRetryCount < 0) || (nRetryCount > 9))
        return( BAD_PARAM );

    // Configuremos el nuevo numero de reintentos
    nCspRetryCount = nRetryCount;

    return( STATUS_OK );
}

/* Esta funcion devuelve el numero de reintentos configurado actualmente */
int Csp32Bridge::cspGetRetryCount()
{
    return (nCspRetryCount);
}


// FUNCIONES PARA DEBUGGEAR

/* Esta funcion obtiene informacion referente al puerto COM especificado en la clase */
int Csp32Bridge::cspGetCommInfo(int nComPort)
{
    DWORD TimeMark;     // Usado para medir el delay de tiempo
    int  i;                // Data leida del puerto serial/modem
    int  portType;     // SERIAL_RS232 | SERIAL_MODEM si el puerto es valido
    int  Next;         // indice para un arreglo
    char *Expect;       // Un puntero a los datos esperados

    portType = SERIAL_RS232;    // Esto asume que estamos en un puerto RS232 valido
    Expect = "OK";              // Esperemos un "OK" si todo va bien
    Next = 0;                   // Indice para Expect[]

    // Veamos si podemos acceder al puerto
    if ( SioReset(nComPort, RX_QUE_SIZE, TX_QUE_SIZE) < 0)
        return( SioWinError(NULL, 0));  // El puerto no es accesible, reportemos el problem

    // Establescamos las lineas de control RTS y CTS
    SioDTR(nComPort,'S');
    SioRTS(nComPort,'S');

    // Enviemos un retorno de acarreo para inicializar el puerto...
    SioPutc(nComPort,'\r');

    // Si esto es un modem, deberia especificar un puerto DSRs
    if (SioDSR(nComPort) != 0)
    {
        /* Un conector DSR envolvente tambien deberia configurar un DSR, asi que debemos verificar
         * el "modem", enviemosle un comando AT*/
        SioPutc(nComPort,'\r');
        Sleep(250);                 // Dejemos que otras cosas corran
        SioPutc(nComPort,'A');
        Sleep(250);                 // Dejemos que otras cosas corran
        SioPutc(nComPort,'T');
        Sleep(250);                 // Dejemos que otras cosas corran
        SioPutc(nComPort,'\r');

        // Si esto es un modem, deberiamos recibir un OK
        Next = 0;
        TimeMark = SioTimer() + 2000;
        while (SioTimer() < TimeMark)
        {
            // Leamos cualquier dato de entrada del puerto serial...
            i = SioGetc(nComPort);
            if (i > -1)
            {
                // Ok, tenemos un caracter, ahora a compararlo con la respuesta esperada...
                if ((char)i == Expect[Next])
                {
                    Next++;
                    if (Next == 2)
                    {
                        portType = SERIAL_MODEM;
                        break;
                    }
                }
            }
        } // Fin del while (Era necesario comentarlo?)
    }

    // Liberemos el puerto ahora que tenemos la informacion
    SioDone(nComPort);

    // Y reportemos que tipo de dispositivo tenemos en frente
    return (portType);
}


// FUNCIONES AVANZADAS

/* Esta funcion se trae los datos mas importantes del dispositivo, leyendo hasta
 * nMaxLength bytes del mismo */

char aUploadCmd[] =
{
    UPLOAD,                  // Comando UPLOAD
    STX,                     // Codigo de opcion
    0x00,                    // Caracter NULL para cerrar los strings
    0x05                     // LRC
};

int Csp32Bridge::cspReadRawData(char aBuffer[], int nMaxLength )
{
    int i, k, n, iSigStart;
    int nRetStatus;
    char j;
    char aSignature[2];

    // Veamos si el campo de firma esta activado..
    nRetStatus = cspGetParam(SEND_SIGNATURE, aSignature, 1);

    // Si no hay respuesta, devolvamos un error
    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // Enviemos el comando (UPLOAD) al lector
    nRetStatus = cspSendCommand (aUploadCmd, sizeof(aUploadCmd));

    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // Hay respuesta, obtengamos el resto del mensajes
    i = 1;

    // Obtengamos el caracter de opcion (STX)
    aByteBuffer[i++] = (char) cspGetc();

    // Obtengamos los caracteres del ID del dispositivo
    for ( j = 0; j < 8 ; j++ )
        aByteBuffer[i++] = szCspDeviceId[j] = (char) cspGetc();

    // Obtengamos la cantidad de caracteres de subida...(Si es una funcion READ, por que hablamos de subida??)
    for ( j = 0; j < 4 ; j++ )
        aByteBuffer[i++] = aCspUploadCount[j] = (char) cspGetc();

    // Leamos las cadenas encontradas hasta el caracter nulo..
    nCspStoredBarcodes = iSigStart = k = n = 0;
    while (( aByteBuffer[i++] = j = (char) cspGetc()) > 0x00 )
    {
        // Obtengamos los caracteres del string del codigo de barras
        iSigStart = k;
        for ( n = 0; n < (int) j; n++ )
            aByteBuffer[i++] = szCspBarData[k++] = (char) cspGetc();
        // Pongamos un NULL entre cada string
        szCspBarData[k++] = 0;
        nCspStoredBarcodes++;
    }

    // "n" tiene la longitud del ultimo string encontrado...
    // Si n tiene exactamente 8 caracteres y la firma esta activada,
    // entonces los ultimos 8 caracteres son la firma
    if (( n == 8 ) && (aSignature[0] == PARAM_ON))
    {
        // Obtengamos los caracteres de la firma del ultimo string encontrado
        for ( k = 0; k < (int)sizeof(aCspSignature); k++ )
            aCspSignature[k] = szCspBarData[iSigStart+k];

        // El string de firma no se cuenta como un codigo de barras!
        nCspStoredBarcodes--;
    }
    else
    {
        // Etiquetemos el campo de firma como desactivado
        memcpy(aCspSignature, "Disabled", sizeof(aCspSignature));
    }

    // Verifiquemos el LRC
    aByteBuffer[i] = cspLrcCheck(aByteBuffer, i);
    if ( aByteBuffer[i] != (char) cspGetc())
    {
        return( COMMAND_LRC_ERROR );
    }

    // Deberiamos copiar los datos al ID de usuario?
    if (aBuffer)
    {
        if (nMaxLength)
            memcpy(aBuffer, aByteBuffer, nMaxLength);
    }

    return(i + 1);
}


/* Esta funcion permite configurar parametros individuales en lugar de hacerlo a traves de
 * una estructura y todos a la vez */
int Csp32Bridge::cspSetParam( int nParam, char szString[], int nMaxLength )
{
    int nRetStatus;
    int i = 0;
    int k;
    char j;

    // Veamos si el dispositivo esta conectado...
    if ((nRetStatus = cspInterrogate()) != STATUS_OK)
        return( nRetStatus );

    // Iniciemos el paquete de descarga
    aByteBuffer[i++] = DOWNLOAD_PARAMETERS;
    aByteBuffer[i++] = STX;

    // Establezcamos la longitud del string encontrado
    aByteBuffer[i++] = nMaxLength + 1;

    // Especifiquemos el numero de parametro
    aByteBuffer[i++] = (char) nParam;

    // Copiemos el string al buffer
    for (k = 0; k < nMaxLength; k++)
        aByteBuffer[i++] = szString[k];

    // Cerremos todos los strings con NULL (se asume que es asi)
    aByteBuffer[i++] = 0x00;

    // Revisemos si el string no esta dañado, usando el LRC
    aByteBuffer[i] = cspLrcCheck( aByteBuffer, i );
    i++;

    // Enviemos el comando al lector
    nRetStatus = cspSendCommand( aByteBuffer, i );

    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // Hay respuesta, obtengamos el mensaje entero...
    i = 1;

    // Obtengamos el caracter de opcion (STX)
    aByteBuffer[i++] = (char) cspGetc();

    // Leamos el string contado hasta el caracter NULL...
    // El primer byte J es la longitud del string contado...
    while (( aByteBuffer[i++] = j = (char) cspGetc()) > 0x00 )
    {
        // Obtengamos el caracter del parametro
        aByteBuffer[i++] = (char) cspGetc();

        // Leamos el valor del parametro en aByteBuffer hasta j-1 caracteres...
        for (k = 0; k < (int) (j-1); k++)
        {
            aByteBuffer[i++] = (char) cspGetc();
        }
    }

    // Verifiquemos el LRC
    aByteBuffer[i] = cspLrcCheck(aByteBuffer, i);
    if ( aByteBuffer[i] != (char) cspGetc())
        return( COMMAND_LRC_ERROR );

    return( STATUS_OK );
}

/* Esta funcion permite leer parametros idividuales del dispositivo uno a la vez */
char aGetParametersCmd[] =
{
    UPLOAD_PARAMETERS,
    STX,

    0x01,                    // Leamos un parametro
    0x00,                    // Parametro TBD

    0x00,                    // Termina el mensaje en NULL
    0x00                     // LRC
};

#define GPC_PARM_START  ((int) 3)
#define GPC_SIZE        (sizeof(aGetParametersCmd) - 1)

int Csp32Bridge::cspGetParam(int nParam, char szString[], int nMaxLength )
{
    int i, k;
    int nRetStatus;
    char j;

    // Veamos si el dispositivo esta conectado
    if ((nRetStatus = cspInterrogate()) != STATUS_OK)
        return( nRetStatus );

    // Llenemos el string del Comando
    aGetParametersCmd[GPC_PARM_START] = (char) nParam;
    aGetParametersCmd[GPC_SIZE] = cspLrcCheck(aGetParametersCmd, GPC_SIZE);

    // Enviemos el comando al dispositivo CSP
    nRetStatus = cspSendCommand (aGetParametersCmd, sizeof(aGetParametersCmd));

    if ( nRetStatus != STATUS_OK )
        return ( nRetStatus );

    // Hemos obtenido respuesta, sigamos
    i = 1;

    // Obtengamos el caracter STX
    aByteBuffer[i++] = (char) cspGetc();

    // Leamos el string contado hasta el caracter NULL,
    // el primer byte "j" es la longitud del string contado
    while (( aByteBuffer[i++] = j = (char) cspGetc()) > 0x00 )
    {
        // Obtengamos el caracter del parametro
        aByteBuffer[i++] = (char) cspGetc();

        // Leamos el valor del parametro en szString[] hasta nMaxLength caracteres
        for (k = 0; k < (int) (j-1); k++)
        {
            if (k < nMaxLength)
                aByteBuffer[i++] = szString[k] = (char) cspGetc();
            // El parametro excedio el tamaño, dejemos de copiarlo en el buffer
            else
                aByteBuffer[i++] = (char) cspGetc();
        }

    }

    // Verifiquemos el LRC
    aByteBuffer[i] = cspLrcCheck(aByteBuffer, i);
    if ( aByteBuffer[i] != (char) cspGetc())
        return( COMMAND_LRC_ERROR );

    return( STATUS_OK );
}

/* Esta funcion permite determinar si el lector esta funcionando y contiene informacion */

char aInterrogateCmd[] =
{
    INTERROGATE,      // Interrogate Command
    STX,              // opcode
    0x00,             // NULL terminate the message
    0x03              // LRC
};

int Csp32Bridge::cspInterrogate()
{
    int i, j;
    int nRetStatus;
    int nCount = nCspRetryCount;

    // Interroga el dispositivo nCspRetrCount + 1 veces...
    do
    {
        // Invalidemos el estado de la ultima interrogacion
        nCspDeviceStatus               = -1;   // No hay estado del dispositivo
        nCspProtocolVersion         = -1;   // No hay version del protocolo
        nCspSystemStatus            = -1;   // No hay estado del sistema
        szCspUserId[0]              =  0;   // No hay ID de usuario
        szCspSwVersion[0]           =  0;   // No hay version del software

        // Aseguremonos que hay un puerto valido seleccionado
        if (nCspActivePort < COM1)
        {
            return( COMMUNICATIONS_ERROR );
        }

        // Enviemos el comando al dispositivo CSP
        nRetStatus = cspSendCommand (aInterrogateCmd, sizeof(aInterrogateCmd));

        // Si el lector devuelve un error, intentemos desde cero...
        if (nRetStatus != STATUS_OK)
        {
            continue;
        }

        // Hemos obtenido respuesta, obtengamos el mensaje entero
        i = 1;

        // Obtengamos el caracter STX
        aByteBuffer[i++] = (char) cspGetc();

        // Obtengamos el caracter de version del protocolo
        nCspProtocolVersion = cspGetc();
        aByteBuffer[i++] = (char) nCspProtocolVersion;

        // Obtengamos el caracter del estado del sistema
        nCspSystemStatus = cspGetc();
        aByteBuffer[i++] = (char) nCspSystemStatus;

        // Obtengamos los caracteres del ID de usuario
        for ( j = 0; j < 8 ; j++ )
            aByteBuffer[i++] = szCspUserId[j] = (char) cspGetc();

        // Obtengamos los caracteres de la version del software
        for ( j = 0; j < 8 ; j++ )
            aByteBuffer[i++] = szCspSwVersion[j] = (char) cspGetc();

        // Obtengamos el caracter NULL
        aByteBuffer[i++] = (char) cspGetc();

        // Verifiquemos el LRC
        aByteBuffer[i] = cspLrcCheck( aByteBuffer, i );
        if ( aByteBuffer[i] != ( unsigned char ) cspGetc())
        {
            nRetStatus = COMMAND_LRC_ERROR;
            continue;
        }

        // El dispositivo fue interrogado exitosamente
        return( STATUS_OK );

    }
    while (nCount--);

    return (nRetStatus);            // Fallo la interrogacion
}

/** **/
/** **/
/** ACA TERMINAN DICHOS METODOS E INICIA CUALQUIERA DE LOS NECESARIOS PARA QT Y SU OBJETO **/
