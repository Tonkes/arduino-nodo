/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package nl.lemval.nododue.util;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import nl.lemval.nododue.NodoDueManager;
import nl.lemval.nododue.cmd.NodoMacroResponse;

/**
 *
 * @author Michael
 */
public class NodoMacroList {

    public static final int MAXLENGTH = 120;
    private ArrayList<NodoMacro> macros;

    public NodoMacroList() {
        macros = new ArrayList<NodoMacro>();
    }

    @SuppressWarnings("element-type-mismatch")
    public boolean remove(Object o) {
        return macros.remove(o);
    }

    public int size() {
        return macros.size();
    }

    public NodoMacro get(int index) {
        if (index >= 0 && index < macros.size()) {
            return macros.get(index);
        }
        return null;
    }

    public boolean move(int index, boolean up) {
        int len = size();
        // If it needs to be moved down, it should be at most the
        // before last item.
        if (!up) {
            len--;
        }

        if (index >= len) {
            return false;
        }
        NodoMacro elem = macros.get(index);
        if (elem == null) {
            return false;
        }

        int swindex = (up ? index - 1 : index + 1);
        NodoMacro swelem = macros.get(swindex);
        macros.remove(index);
        macros.add(index, swelem);
        macros.remove(swindex);
        macros.add(swindex, elem);
        return true;
    }

    public boolean add(NodoMacro nodoMacro) {
        if (size() >= MAXLENGTH) {
            return false;
        }
        for (NodoMacro elem : macros) {
            if (elem.equals(nodoMacro)) {
                return false;
            }
        }
        return macros.add(nodoMacro);
    }

    public boolean load(File selected) {

        BufferedReader reader = null;
        boolean loadAll = true;
        try {
            macros.clear();
            reader = new BufferedReader(new FileReader(selected));
            String line;
            while ((line = reader.readLine()) != null) {
                NodoMacroResponse[] readMacro = NodoMacroResponse.getMacros(line);
                for (int i = 0; i < readMacro.length; i++) {
                    NodoMacroResponse response = readMacro[i];
                    NodoMacro macro = new NodoMacro(response.getEvent(), response.getAction());
                    if (macro != null) {
                        loadAll = loadAll && add(macro);
                    }
                }
            }
            if (!loadAll) {
                NodoDueManager.showDialog("Macroloader.load_incomplete");
            }
            return true;
        } catch (IOException e) {
            NodoDueManager.showDialog("Macroloader.load_failed", selected, e.getMessage(), e);
        } finally {
            try {
                reader.close();
            } catch (Exception e) {
            }
        }
        return false;
    }

    public void save(File selected, boolean overwrite) {
        if (selected.exists() && !overwrite) {
            selected.renameTo(new File(selected.getPath() + ".bak"));
        }
        String NEWLINE = System.getProperty("line.separator");
        BufferedWriter writer = null;
        try {
            writer = new BufferedWriter(new FileWriter(selected));
            int cnt = 1;
            for (NodoMacro nodoMacro : macros) {
                writer.write(new NodoMacroResponse(cnt++, nodoMacro).toString());
                writer.write(NEWLINE);
            }
        } catch (IOException e) {
            NodoDueManager.showDialog("Macroloader.save_failed", selected, e.getMessage(), e);
        } finally {
            try {
                writer.close();
            } catch (Exception e) {
            }
        }
    }
}
