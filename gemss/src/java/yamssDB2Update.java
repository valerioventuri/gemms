/*
############################################################################
# Copyright 2008-2012 Istituto Nazionale di Fisica Nucleare
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
############################################################################
*/

import java.sql.*;
import javax.sql.*;

public class yamssDB2Update {

  public static void main(String[] args) {

    String serverName, dbName, user, password, query;
    int portNumber=0;

    if (args.length != 6) {
      System.err.println("Usage: yamssDB2Delete serverName dbName portNumber user password \"query\"");
      System.exit(1);
    }

    serverName = args[0];
    dbName = args[1];
    password = args[4];
    query = args[5];
    user = args[3];

    try {
      portNumber = Integer.parseInt(args[2]);
    } catch (NumberFormatException e) {
      System.err.println("port number must be an integer");
      System.exit(1);
    }
    
    connectWithDataSource(serverName, dbName, portNumber, user, password, query);
  }

  private static void connectWithDataSource(String serverName, String dbName, Integer portNumber, String user, String password, String query) {

    Connection c = null;
    Statement s = null;
    int r = 0;

    try {
      DataSource ds = 
       new com.ibm.db2.jcc.DB2SimpleDataSource();
      ((com.ibm.db2.jcc.DB2BaseDataSource) ds).setServerName(serverName);
      ((com.ibm.db2.jcc.DB2BaseDataSource) ds).setPortNumber(portNumber);
      ((com.ibm.db2.jcc.DB2BaseDataSource) ds).setDatabaseName(dbName);
      ((com.ibm.db2.jcc.DB2BaseDataSource) ds).setDriverType(4); 

      c = ds.getConnection(user, password);

      s = c.createStatement();
      r = s.executeUpdate(query);

      System.out.println("Rows involved in query: "+r);

      s.close();
      c.close();

    } catch(java.sql.SQLException e) {
       System.err.println("SQLException: " + e.getMessage());
       System.exit(1);
    }

  }

}
