#ifndef PORTMAP_HPP
#define PORTMAP_HPP

/**
 * This port is used by the login server.
 */
#define PORT_LOGIN				20013

/**
 * This port is used by the machined process.
 */
#define PORT_MACHINED_OLD		20014
#define PORT_MACHINED			20018

/**
 * Port used by machined during the initial interface discovery.
 */
#define PORT_BROADCAST_DISCOVERY	20019

/**
 * Cell python servers are at PORT_PYTHON_BASEAPP + BaseApp id (TCP).
 */
#define PORT_PYTHON_BASEAPP		40000

/**
 * Cell python servers are at PORT_PYTHON_CELLAPP + CellApp id (TCP).
 */
#define PORT_PYTHON_CELLAPP		50000

/**
 * Cell python servers are at PORT_PYTHON_SERVICEAPP + ServiceApp id (TCP).
 */
#define PORT_PYTHON_SERVICEAPP		60000

#endif // PORTMAP_HPP
