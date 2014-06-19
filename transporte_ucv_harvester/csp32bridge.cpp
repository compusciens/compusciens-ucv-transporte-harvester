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

}

//Funcion que permite inicializar todos los parametros de la clase
void Csp32Bridge::cspInitParms()
{
    nCspActivePort              = -1;   // no comm port selected
    nCspDeviceStatus            = -1;   // no current Device status
    nCspProtocolVersion         = -1;   // no protocol version available
    nCspSystemStatus            = -1;   // no system status available

    nCspStoredBarcodes          =  0;   // number of stored barcode strings
}

int Csp32Bridge::cspRestore()
{
    int nRetStatus = COMMUNICATIONS_ERROR;

    // Estamos cerrando un puerto valido?
    if (nCspActivePort >= COM1)
    {
        // Cierra cualquier puerto abierto previamente
        nRetStatus = SioDone(nCspActivePort);
    }

    // Inicializa la interfaz de la clase
    cspInitParms();

    // Y retorna el estado de la operacion (mas bien del dispositivo segun parece)
    if (nRetStatus < 0)
        return (COMMUNICATIONS_ERROR);
    else
        return (STATUS_OK);
}

int Csp32Bridge::cspInit(int nComPort)
{
    // Nos aseguramos que el puerto escogido sea el correcto, igual esto redunda aca
    if (nComPort >= COM1)
    {
        // Cierra cualquier puerto abierto previamente
        if (nCspActivePort >= COM1)
            cspRestore();

        // Ahora, intentemos inicializar el puerto escogido
        if (SioReset(nComPort, RX_QUE_SIZE, TX_QUE_SIZE) < 0)
        {
            // Esto indica que ningun puerto ha sido activado
            return(nCspActivePort = COMMUNICATIONS_ERROR);
        }
        else
        {
            // Establece el puerto especificado como el puerto activo actual
            nCspActivePort = nComPort;
        }

        // Establece los parametros del puerto para la compatibilidad con el dispositivo CSP
        SioParms(nCspActivePort, OddParity, OneStopBit, WordLength8);
        SioBaud(nCspActivePort, Baud2400);
        SioDTR(nCspActivePort, SET_LINE);
        SioRTS(nCspActivePort, SET_LINE);
        return (STATUS_OK);
    }

    return (BAD_PARAM);
}
