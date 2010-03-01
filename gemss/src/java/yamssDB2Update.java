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
