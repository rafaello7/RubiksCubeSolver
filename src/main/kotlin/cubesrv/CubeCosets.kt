package cubesrv

/* A set of spaces (cosets of BG space) reachable at specific depth.
 * A coset is identified by a representative cube.
 */
class CubeCosetsAtDepth {
    // Map<edges of representative cube, cubes reachable at depth>
    // the representative cube corners permutation is identity; 2^7 corner orientations
    val m_itemsArr = Array<MutableMap<cubeedges, MutableList<cube>>>(2187) {
        mutableMapOf<cubeedges, MutableList<cube>>() }

    /* Adds a cube to coset identified by corner orients and representative cubeedges.
     */
    fun addCube(ccoReprIdx : Int, ceRepr : cubeedges, c : cube) : Boolean {
        val items : MutableMap<cubeedges, MutableList<cube>> = m_itemsArr[ccoReprIdx]
        val cubeList : MutableList<cube> = items.getOrPut(ceRepr) { mutableListOf<cube>() }
        val res : Boolean = cubeList.isEmpty()
        cubeList.add(c)
        return res
    }

    fun containsCCOrients(ccoReprIdx : Int) : Boolean {
        return !m_itemsArr[ccoReprIdx].isEmpty()
    }

    fun getCubesForCE(ccoReprIdx : Int, ceRepr : cubeedges) : List<cube>?  {
        val items : MutableMap<cubeedges, MutableList<cube>> = m_itemsArr[ccoReprIdx]
        return items.get(ceRepr)
    }
}

class CubeCosets(size : Int) {
    val m_cubesAtDepths = Array<CubeCosetsAtDepth>(size) { CubeCosetsAtDepth(); }
    var m_availCount : Int = 0

    fun availCount() : Int = m_availCount
    fun incAvailCount() { ++m_availCount; }
    fun availMaxCount() : Int = m_cubesAtDepths.size
    fun getAt(idx : Int) = m_cubesAtDepths[idx]
}
