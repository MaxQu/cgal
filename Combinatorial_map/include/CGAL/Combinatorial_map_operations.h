// Copyright (c) 2010-2011 CNRS and LIRIS' Establishments (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
// Author(s)     : Guillaume Damiand <guillaume.damiand@liris.cnrs.fr>
//
#ifndef CGAL_COMBINATORIAL_MAP_OPERATIONS_H
#define CGAL_COMBINATORIAL_MAP_OPERATIONS_H 1

#include <CGAL/Combinatorial_map_basic_operations.h>
#include <vector>
#include <stack>

namespace CGAL {

  /** @file Combinatorial_map_operations.h
   * Some operations to modify a combinatorial map.
   */

  /** Insert a vertex in the given 2-cell which is splitted in triangles,
   * once for each inital edge of the facet.
   * @param amap the used combinatorial map.
   * @param adart a dart of the facet to triangulate.
   * @return A dart incident to the new vertex.
   */
  template < class Map >
  typename Map::Dart_handle
  insert_cell_0_in_cell_2(Map& amap, typename Map::Dart_handle adart)
  {
    CGAL_assertion(adart != NULL && adart!=Map::null_dart_handle);

    typename Map::Dart_handle first = adart, prev = NULL, cur = NULL;
    typename Map::Dart_handle n1 = NULL, n2 = NULL;

    typename Map::Dart_handle nn1 = NULL, nn2 = NULL;

    // If the facet is open, we search the dart 0-free
    while (!first->is_free(0) && first->beta(0) != adart)
      first = first->beta(0);

    // Stack of couple of dart and dimension for which
    // we must call on_split functor
    std::stack<internal::Couple_dart_and_dim<typename Map::Dart_handle> >
      tosplit;

    // Mark used to mark darts already treated.
    int treated = amap.get_new_mark();

    // Stack of marked darts
    std::stack<typename Map::Dart_handle> tounmark;

    // Now we run through the facet
    for (CGAL::CMap_dart_iterator_basic_of_orbit<Map,1> it(amap,first);
         it.cont();)
    {
      cur = it;
      ++it;
      amap.mark(cur, treated);
      tounmark.push(cur);

      if ( cur!=first )
      {
        if ( amap.template degroup_attribute_of_dart<2,
             typename Map::template Dart_of_involution_range<1> >
             (first, cur) )
          tosplit.push(internal::Couple_dart_and_dim
                       <typename Map::Dart_handle>
                       (first,cur,2));
      }

      if (!cur->is_free(0))
      {
        n1  = amap.create_dart();
        amap.link_beta_0(cur, n1);
      }
      else n1 = NULL;

      if (!cur->is_free(1))
      {
        n2 = amap.create_dart();
        amap.link_beta_1(cur, n2);
      }
      else n2 = NULL;

      if (n1 != NULL && n2 != NULL)
        amap.link_beta_0(n1, n2);

      if (n1 != NULL && prev != NULL)
        amap.link_beta_for_involution(prev, n1, 2);

      for (unsigned int dim=3; dim<=Map::dimension; ++dim)
      {
        if ( !adart->is_free(dim) )
        {
          if ( !amap.is_marked(cur->beta(dim), treated) )
          {
            if (n1!=NULL)
            {
              nn1=amap.create_dart();
              amap.link_beta_1(cur->beta(dim), nn1);
              amap.link_beta_for_involution(n1, nn1, dim);
            }
            else nn1=NULL;

            if (n2!=NULL)
            {
              nn2=amap.create_dart();
              amap.link_beta_0(cur->beta(dim), nn2);
              amap.link_beta_for_involution(n2, nn2, dim);
            }
            else nn2=NULL;

            if (nn1 != NULL && nn2 != NULL)
              amap.basic_link_beta_1(nn1, nn2);

            if (nn1 != NULL && prev != NULL)
              amap.link_beta_for_involution(nn1, prev->beta(dim), 2);

            amap.mark(cur->beta(dim), treated);
            tounmark.push(cur->beta(dim));
          }
          else
          {
            if ( n1!=NULL )
              amap.link_beta_for_involution(n1, cur->beta(dim)->beta(1), dim);
            if ( n2!=NULL )
              amap.link_beta_for_involution(n2, cur->beta(dim)->beta(0), dim);
          }
        }
      }

      prev = n2;
    }

    if (n2 != NULL)
    {
      amap.link_beta_for_involution(first->beta(0), n2, 2);
      for (unsigned int dim=3; dim<=Map::dimension; ++dim)
      {
        if ( !adart->is_free(dim) )
        {
          amap.link_beta_for_involution(first->beta(0)->beta(dim),
                                        n2->beta(dim), 2);
        }
      }
    }

    // Now we unmark all marked darts
    while ( !tounmark.empty() )
    {
      amap.unmark(tounmark.top(), treated);
      tounmark.pop();
    }

    CGAL_assertion(amap.is_whole_map_unmarked(treated));
    amap.free_mark(treated);

    while ( !tosplit.empty() )
    {
      internal::Couple_dart_and_dim<typename Map::Dart_handle> c=tosplit.top();
      tosplit.pop();
      internal::Call_split_functor<Map, 2>::run(c.d1, c.d2);
    }

    CGAL_expensive_postcondition( amap.is_valid() );

    return n1;
  }

  /** Test if an i-cell can be removed.
   *  An i-cell can be removed if i==Map::dimension or i==Map::dimension-1,
   *     or if there are at most two (i+1)-cell incident to it.
   * @param adart a dart of the i-cell.
   * @return true iff the i-cell can be removed.
   */
  template < class Map, unsigned int i >
  bool is_removable(const Map& amap, typename Map::Dart_const_handle adart)
  {
    CGAL_assertion(adart != NULL);
    CGAL_static_assertion(0<=i && i<=Map::dimension);

    if ( i==Map::dimension   ) return true;
    if ( i==Map::dimension-1 ) return true;

    // TODO ? Optimisation for dim-2, and to not test all
    // the darts of the cell ?
    bool res = true;
    for (CMap_dart_const_iterator_of_cell<Map,i> it(amap, adart);
         res && it.cont(); ++it)
    {
      if (it->beta(i+2)->beta(i+1) != it->beta_inv(i+1)->beta(i+2) )
        res = false;
    }
    return res;
  }

  /** Remove an i-cell, 0<i<dimension, and merge eventually both incident
   *  (i+1)-cells.
   *  @param amap the used combinatorial map.
   *  @param adart a dart of the i-cell to remove.
   *  @return the number of deleted darts.
   */
  template<class Map, unsigned int i, unsigned int nmi>
  struct Remove_cell_functor
  {
    static size_t run(Map& amap, typename Map::Dart_handle adart)
    {
      CGAL_static_assertion ( 1<=i && i<Map::dimension );
      CGAL_assertion( (is_removable<Map,i>(amap, adart)) );

      size_t res = 0;

      typename Map::Dart_handle d1, d2;
      typename Map::Dart_handle dg1=NULL, dg2=NULL;

      int mark = amap.get_new_mark();
      int mark_modified_darts = amap.get_new_mark();

      std::deque<typename Map::Dart_handle> to_erase;

      const int iinv = CGAL_BETAINV(i);

      /*int mark_for_incident_cells[Map::Helper::nb_attribs];
      std::deque<std::deque<typename Map::Dart_handle> >
          incident_cells[Map::Helper::nb_attribs];

      // Marks used to mark all the incident cells.
      for (int j=0; j<Map::Helper::nb_attribs; ++j)
      {
        mark_for_incident_cells [j] = amap.get_new_mark();
        CGAL_assertion( mark_for_incident_cells[j]!=-1 );
      }*/

      // First we store and mark all the darts of the i-cell to remove.
      for ( CMap_dart_iterator_basic_of_cell<Map,i> it(amap,adart,mark);
            it.cont(); ++it )
      {
        to_erase.push_back(it);
        if ( !it->is_free(i+1) && dg1==NULL )
        { dg1=it; dg2=it->beta(i+1); }
        amap.mark(it, mark);
        ++res;
      }

      // Second we store all the incident cells that can be split by
      // the operation.
      typename std::deque<typename Map::Dart_handle>::iterator it =
        to_erase.begin();
      /*for (; it != to_erase.end(); ++it)
      {
        d1 = (*it)->beta(iinv);
        while ( d1!=Map::null_dart_handle && amap.is_marked(d1, mark) )
        {
          d1 = d1->beta(i+1)->beta(iinv);
          if (d1 == (*it)->beta(iinv)) d1 = Map::null_dart_handle;
        }

        d2 = (*it)->beta(i+1)->beta(i);
        while ( d2!=Map::null_dart_handle && amap.is_marked(d2, mark) )
        {
          d2 = d2->beta(i+1)->beta(i);
          if ( d2==(*it)->beta(i+1)->beta(i) ) d2=Map::null_dart_handle;
        }

        if (d1 != Map::null_dart_handle)
        {
          if (d2 != Map::null_dart_handle)
          {
            if ( dg1==NULL ) { dg1 = d1; dg2=d2; }
            Map::Helper::template Foreach_enabled_attributes
                <internal::Store_incident_cells<Map,i> >::
                run(&amap, d1, mark, &mark_for_incident_cells[0],
                    &incident_cells[0]);
            Map::Helper::template Foreach_enabled_attributes
                <internal::Store_incident_cells<Map,i> >::
                run(&amap, d2, mark, &mark_for_incident_cells[0],
                    &incident_cells[0]);
          }
          else
          {
            if ( !d1->is_free(i) )
            {
              Map::Helper::template Foreach_enabled_attributes
                  <internal::Store_incident_cells<Map,i> >::
                  run(&amap, d1->beta(i), mark, &mark_for_incident_cells[0],
                      &incident_cells[0]);
            }
          }
        }
        else if (d2 != Map::null_dart_handle)
        {
          if ( !d2->is_free(iinv) )
          {
            Map::Helper::template Foreach_enabled_attributes
                <internal::Store_incident_cells<Map,i> >::
                run(&amap, d2, mark, &mark_for_incident_cells[0],
                    &incident_cells[0]);
          }
        }

        if ((*it)->is_free(i+1) && !(*it)->is_free(i))
        {
          d1 = (*it)->beta(i);
          if ( !d1->is_free(iinv) )
          {
            Map::Helper::template Foreach_enabled_attributes
                <internal::Store_incident_cells<Map,i> >::
                run(&amap, d1, mark, &mark_for_incident_cells[0],
                    &incident_cells[0]);
          }
        }

        // Required to process cells non consider in the cases above.
        amap.update_dart_of_all_attributes(*it, mark);
      }*/

      for (; it != to_erase.end(); ++it)
        amap.update_dart_of_all_attributes(*it, mark);

      // We group the two (i+1)-cells incident if they exist.
      if ( dg1!=NULL )
        amap.template group_attribute<i+1>(dg1, dg2);

      std::deque<typename Map::Dart_handle> modified_darts;
      // std::deque<typename Map::Dart_handle> modified_darts2;

      // For each dart of the i-cell, we modify i-links of neighbors.
      for ( it=to_erase.begin(); it != to_erase.end(); ++it)
      {
        d1 = (*it)->beta(iinv);
        while ( d1!=Map::null_dart_handle && amap.is_marked(d1, mark) )
        {
          d1 = d1->beta(i+1)->beta(iinv);
          if (d1 == (*it)->beta(iinv)) d1 = Map::null_dart_handle;
        }

        if ( !amap.is_marked(d1, mark_modified_darts) )
        {
          d2 = (*it)->beta(i+1)->beta(i);
          while ( d2!=Map::null_dart_handle && amap.is_marked(d2, mark) )
          {
            d2 = d2->beta(i+1)->beta(i);
            if ( d2==(*it)->beta(i+1)->beta(i) ) d2=Map::null_dart_handle;
          }

          if ( !amap.is_marked(d2, mark_modified_darts) )
          {
            if (d1 != Map::null_dart_handle)
            {
              if (d2 != Map::null_dart_handle && d1!=d2 )
              {
                //d1->basic_link_beta(d2, i);
                amap.template basic_link_beta<i>(d1, d2);
                amap.mark(d1, mark_modified_darts);
                amap.mark(d2, mark_modified_darts);
                modified_darts.push_back(d1);
                modified_darts.push_back(d2);
                /*if ( i==1 )
                {
                  d2->basic_link_beta(d1, 0);
                  modified_darts.push_back(d2);
                }*/
                //            modified_darts2.push_back(d1);
              }
              else
              {
                if ( !d1->is_free(i) )
                {
                  d1->unlink_beta(i);
                  CGAL_assertion( !amap.is_marked(d1, mark_modified_darts) );
                  amap.mark(d1, mark_modified_darts);
                  modified_darts.push_back(d1);
                }
              }
            }
            else if (d2 != Map::null_dart_handle)
            {
              if ( !d2->is_free(iinv) )
              {
                d2->unlink_beta(iinv);
                CGAL_assertion( !amap.is_marked(d2, mark_modified_darts) );
                amap.mark(d2, mark_modified_darts);
                modified_darts.push_back(d2);
              }
            }
          }
        }
        if ( (*it)->is_free(i+1) && !(*it)->is_free(i) )
        {
          d1 = (*it)->beta(i);
          if ( !d1->is_free(iinv) )
          {
            d1->unlink_beta(iinv);
            CGAL_assertion( !amap.is_marked(d1, mark_modified_darts) );
            amap.mark(d1, mark_modified_darts);
            modified_darts.push_back(d1);
          }
        }
      }

      // We test the split of all the incident cells for all the non
      // void attributes.
      Map::Helper::template Foreach_enabled_attributes
          <internal::Test2_split_with_deque<Map,i> >::
          //      <internal::Test_split_with_deque<Map,i> >::
                run(&amap, &modified_darts, mark_modified_darts);
                    //&modified_darts2);
                    //&mark_for_incident_cells[0], &incident_cells[0]);

      // We remove all the darts of the i-cell.
      for (  it=to_erase.begin(); it!=to_erase.end(); ++it )
      { amap.erase_dart(*it); }

      CGAL_assertion( amap.is_whole_map_unmarked(mark) );
      amap.free_mark(mark);

      if ( !amap.is_whole_map_unmarked(mark_modified_darts) )
      {
        for ( typename std::deque<typename Map::Dart_handle>::
              iterator it=modified_darts.begin();
              it!=modified_darts.end(); ++it )
          amap.unmark(*it, mark_modified_darts);
        /*for ( typename std::deque<typename Map::Dart_handle>::
              iterator it=modified_darts2.begin();
              it!=modified_darts2.end(); ++it )
          amap.unmark(*it, mark_modified_darts);*/
      }

      CGAL_assertion ( amap.is_whole_map_unmarked(mark_modified_darts) );
      amap.free_mark(mark_modified_darts);

      // We free the marks.
      /*for (int j=0; j<Map::Helper::nb_attribs; ++j)
      {
        CGAL_assertion( amap.is_whole_map_marked
                        (mark_for_incident_cells[j]) );
        amap.free_mark( mark_for_incident_cells[j] );
      }*/

      CGAL_expensive_postcondition( amap.is_valid() );
      assert( amap.is_valid() );

      return res;
    }
  };

  /** Remove a d-cell, in a d-map (special case).
   *  @param amap the used combinatorial map.
   *  @param adart a dart of the volume to remove.
   *  @return the number of deleted darts.
   */
  template<class Map,unsigned int i>
  struct Remove_cell_functor<Map,i,0>
  {
    static size_t run(Map& amap, typename Map::Dart_handle adart)
    {
      CGAL_assertion( adart!=NULL );

      int mark = amap.get_new_mark();
      std::deque<typename Map::Dart_handle> to_erase;
      size_t res = 0;

      std::deque<typename Map::Dart_handle> modified_darts;

/*      int mark_for_incident_cells[Map::Helper::nb_attribs];
      std::deque<std::deque<typename Map::Dart_handle> >
          incident_cells[Map::Helper::nb_attribs];

      // Marks used to mark all the incident cells.
      for (int j=0; j<Map::Helper::nb_attribs; ++j)
      {
        mark_for_incident_cells[j] = amap.get_new_mark();
        CGAL_assertion( mark_for_incident_cells[j]!=-1 );
      }
*/

      // 1) We mark all the darts of the d-cell.
      for (CMap_dart_iterator_basic_of_cell<Map,Map::dimension>
           it(amap,adart,mark); it.cont(); ++it)
      {
        to_erase.push_back(it);
        amap.mark(it,mark);
        ++res;
      }

      typename std::deque<typename Map::Dart_handle>::iterator
        it = to_erase.begin();

      // 2) We store all the incident cells that can be split by the operation,
      // and we update the dart of the cells incident to the remove volume.
/*      for ( ; it != to_erase.end(); ++it )
      {
        if ( !(*it)->is_free(Map::dimension) )
        {
          Map::Helper::template Foreach_enabled_attributes
              <internal::Store_incident_cells<Map,i> >::
              run(&amap, *it, mark, &mark_for_incident_cells[0],
                  &incident_cells[0]);
        }
        amap.update_dart_of_all_attributes(*it, mark);
      }*/


      // 3) We unlink all the darts of the volume for beta-d.
      for ( it = to_erase.begin(); it != to_erase.end(); ++it )
      {
        if ( !(*it)->is_free(Map::dimension) &&
             !amap.is_marked((*it)->beta(Map::dimension), mark) )
        {
          modified_darts.push_back((*it)->beta(Map::dimension));
          //(*it)->beta(Map::dimension)->unlink_beta(Map::dimension);
          amap.template unlink_beta_for_involution(*it, Map::dimension);
        }
      }

      // 4) We test the split of all the incident cells for all the non
      // void attributes.
      Map::Helper::template Foreach_enabled_attributes
                <internal::Test2_split_with_deque<Map,i> >::
                run(&amap, &modified_darts, -1); //, modified_darts2);
                    //&mark_for_incident_cells[0],
                    //&incident_cells[0]);

      // 5) We remove all the darts of the d-cell.
      for ( it = to_erase.begin(); it != to_erase.end(); ++it )
      { amap.erase_dart(*it); }

      CGAL_assertion( amap.is_whole_map_unmarked(mark) );
      amap.free_mark(mark);

      // We free the marks.
/*      for (int j=0; j<Map::Helper::nb_attribs; ++j)
      {
        CGAL_assertion( amap.is_whole_map_marked
                        (mark_for_incident_cells [j]) );
        amap.free_mark( mark_for_incident_cells [j] );
      }*/

      CGAL_expensive_postcondition( amap.is_valid() );
      assert( amap.is_valid() ); // TO REMOVE

      return res;
    }
  };

  /** Remove a vertex, and merge eventually both incident edges.
   * @param amap the used combinatorial map.
   * @param adart a dart of the vertex to remove.
   * @return the number of deleted darts.
   */
  template<class Map,unsigned int nmi>
  struct Remove_cell_functor<Map,0,nmi>
  {
    static size_t run(Map& amap, typename Map::Dart_handle adart)
    {
      CGAL_assertion( (is_removable<Map,0>(amap,adart)) );

      size_t res = 0;

      typename Map::Dart_handle d1, d2;
      typename Map::Dart_handle dg1=NULL, dg2=NULL;

      int mark = amap.get_new_mark();
//      int mark_modified_darts = amap.get_new_mark();

      std::deque<typename Map::Dart_handle> to_erase;

      std::deque<typename Map::Dart_handle> modified_darts;

/*      int mark_for_incident_cells[Map::Helper::nb_attribs];
      std::deque<std::deque<typename Map::Dart_handle> >
          incident_cells[Map::Helper::nb_attribs];

      // Marks used to mark all the incident cells.
      for (int j=0; j<Map::Helper::nb_attribs; ++j)
      {
        mark_for_incident_cells [j] = amap.get_new_mark();
        CGAL_assertion( mark_for_incident_cells [j]!=-1 );
      }
*/

      // First we store and mark all the darts of the 0-cell to remove.
      for ( CMap_dart_iterator_basic_of_cell<Map,0> it(amap,adart,mark);
            it.cont(); ++it )
      {
        to_erase.push_back(it);
        if ( !it->is_free(0) && dg1==NULL )
        { dg1=it; dg2=it->beta(0); }
        amap.mark(it, mark);
        ++res;
      }

      // Second we store all the incident cells that can be split by
      // the operation.
      typename std::deque<typename Map::Dart_handle>::iterator it =
        to_erase.begin();
/*      for (; it != to_erase.end(); ++it)
      {
        if ( !(*it)->is_free(0) )
        {
          if ( !(*it)->is_free(1) && (*it)->beta(0)!=(*it) )
          {
            if ( dg1==NULL ) { dg1=(*it)->beta(0); dg2=(*it)->beta(1); }
            Map::Helper::template Foreach_enabled_attributes
                <internal::Store_incident_cells<Map,0> >::
                run(&amap, *it, mark, &mark_for_incident_cells[0],
                    &incident_cells[0]);
          }
          else
          {
            Map::Helper::template Foreach_enabled_attributes
                <internal::Store_incident_cells<Map,0> >::
                run(&amap, *it, mark, &mark_for_incident_cells[0],
                    &incident_cells[0]);
          }
        }
        else
        {
          if ( !(*it)->is_free(1) )
          {
            Map::Helper::template Foreach_enabled_attributes
                <internal::Store_incident_cells<Map,0> >::
                run(&amap, (*it)->beta(1), mark, &mark_for_incident_cells[0],
                    &incident_cells[0]);
          }
        }
      }
*/

      for (; it != to_erase.end(); ++it)
        amap.update_dart_of_all_attributes(*it, mark);

      // We group the two edges incident if they exist.
      if ( dg1!=NULL )
        amap.template group_attribute<1>(dg1, dg2);

      // For each dart of the vertex, we modify 0 and 1-links of neighbors.
      for ( it=to_erase.begin(); it != to_erase.end(); ++it)
      {
        if ( !(*it)->is_free(0) )
        {
          if ( !(*it)->is_free(1) && (*it)->beta(0)!=(*it) )
          {
            amap.template basic_link_beta<1>((*it)->beta(0), (*it)->beta(1));
            modified_darts.push_back((*it)->beta(0));
            modified_darts.push_back((*it)->beta(1));
          }
          else
          {
            (*it)->beta(0)->unlink_beta(1);
            modified_darts.push_back((*it)->beta(0));
          }

          for ( unsigned int j=2; j<=Map::dimension; ++j )
          {
            if ( !(*it)->is_free(j) )
            {
              // TODO push these darts in modified_darts ?
              // not sure this is required
              amap.basic_link_beta((*it)->beta(0), (*it)->beta(j), j);
            //((*it)->beta(0))->basic_link_beta((*it)->beta(j),j);
            }
          }
        }
        else
        {
          if ( !(*it)->is_free(1) )
          {
            (*it)->beta(1)->unlink_beta(0);
            modified_darts.push_back((*it)->beta(1));
          }

          for ( unsigned int j=2; j<=Map::dimension; ++j )
          {
            if ( !(*it)->is_free(j) )
            {
              // TODO push these darts in modified_darts ?
              // not sure this is required
              amap.unlink_beta(*it, j);
            }
          }
        }
      }

      // We test the split of all the incident cells for all the non
      // void attributes.
      Map::Helper::template Foreach_enabled_attributes
          <internal::Test2_split_with_deque<Map,0> >::
               // <internal::Test_split_with_deque<Map,0> >::
                run(&amap,
                    &modified_darts, -1); //mark_modified_darts);
                    //&mark_for_incident_cells[0],
                    //&incident_cells[0]);

      // We remove all the darts of the i-cell.
      for (  it=to_erase.begin(); it!=to_erase.end(); ++it )
      { amap.erase_dart(*it); }

      CGAL_assertion( amap.is_whole_map_unmarked(mark) );
      amap.free_mark(mark);

      // We free the marks.
 /*     for (int j=0; j<Map::Helper::nb_attribs; ++j)
      {
        CGAL_assertion( amap.is_whole_map_marked
                        (mark_for_incident_cells [j]) );
        amap.free_mark( mark_for_incident_cells [j] );
      }
      */

      CGAL_expensive_postcondition( amap.is_valid() );
      assert( amap.is_valid() ); // TO REMOVEE

      return res;
    }
  };

  /** Remove an i-cell, 0<=i<=dimension.
   * @param amap the used combinatorial map.
   * @param adart a dart of the i-cell to remove.
   * @return the number of deleted darts.
   */
  template < class Map, unsigned int i >
  size_t remove_cell(Map& amap, typename Map::Dart_handle adart)
  { return Remove_cell_functor<Map,i,Map::dimension-i>::run(amap,adart); }

  /** Test if an edge can be inserted onto a 2-cell between two given darts.
   * @param amap the used combinatorial map.
   * @param adart1 a first dart.
   * @param adart2 a second dart.
   * @return true iff an edge can be inserted between adart1 and adart2.
   */
  template < class Map >
  bool is_insertable_cell_1_in_cell_2(const Map& amap,
                                      typename Map::Dart_const_handle adart1,
                                      typename Map::Dart_const_handle adart2)
  {
    CGAL_assertion(adart1 != NULL && adart2 != NULL);
    if ( adart1==adart2 ) return false;
    for ( CMap_dart_const_iterator_of_orbit<Map,1> it(amap,adart1);
          it.cont(); ++it )
    {
      if ( it==adart2 )  return true;
    }
    return false;
  }

  /** Test if an i-cell can be contracted.
   *  An i-cell can be contracted if i==1
   *     or if there are at most two (i-1)-cell incident to it.
   * @param adart a dart of the i-cell.
   * @return true iff the i-cell can be contracted.
   */
  template < class Map, unsigned int i >
  bool is_contractible(const Map& amap, typename Map::Dart_const_handle adart)
  {
    CGAL_assertion(adart != NULL);
    CGAL_static_assertion(0<=i && i<=Map::dimension);

    if ( i==0 ) return false;
    if ( i==1 ) return true;

    // TODO ? Optimisation possible to not test all
    // the darts of the cell ?
    bool res = true;
    for (CMap_dart_const_iterator_of_cell<Map,i> it(amap, adart);
         res && it.cont(); ++it)
    {
      if (it->beta(i-2)->beta(i-1) != it->beta_inv(i-2)->beta(i-1) )
        res = false;
    }
    return res;
  }

  /** Contract an i-cell, 1<i<=dimension, and merge eventually both incident
   *  (i-1)-cells.
   * @param amap the used combinatorial map.
   * @param adart a dart of the i-cell to contract.
   * @return the number of deleted darts.
   */
  template<class Map, unsigned int i>
  struct Contract_cell_functor
  {
    static size_t run(Map& amap, typename Map::Dart_handle adart)
    {
      CGAL_static_assertion ( 2<=i && i<=Map::dimension );
      CGAL_assertion( (is_contractible<Map,i>(amap, adart)) );

      size_t res = 0;

      typename Map::Dart_handle d1, d2;
      typename Map::Dart_handle dg1=NULL, dg2=NULL;

      int mark = amap.get_new_mark();
      int mark_modified_darts = amap.get_new_mark();

      std::deque<typename Map::Dart_handle> to_erase;

      const int imuinv = CGAL_BETAINV(i-1);

      // First we store and mark all the darts of the i-cell to contract.
      for ( CMap_dart_iterator_basic_of_cell<Map,i> it(amap,adart,mark);
            it.cont(); ++it )
      {
        to_erase.push_back(it);
        if ( !it->is_free(i-1) && dg1==NULL )
        { dg1=it; dg2=it->beta(i-1); }
        amap.mark(it, mark);
        ++res;
      }

      // We group the two (i+1)-cells incident if they exist.
      if ( dg1!=NULL )
        amap.template group_attribute<i-1>(dg1, dg2);

      // Second we update the dart of the cell attributes on non marked darts.
      typename std::deque<typename Map::Dart_handle>::iterator it =
          to_erase.begin();
      for (; it != to_erase.end(); ++it)
        amap.update_dart_of_all_attributes(*it, mark);

      std::deque<typename Map::Dart_handle> modified_darts;

      // For each dart of the i-cell, we modify i-links of neighbors.
      for ( it=to_erase.begin(); it!=to_erase.end(); ++it )
      {
        d1 = (*it)->beta(i);
        while ( d1!=Map::null_dart_handle && amap.is_marked(d1, mark) )
        {
          d1 = d1->beta(imuinv)->beta(i);
          if (d1 == (*it)->beta(i)) d1 = Map::null_dart_handle;
        }

        if ( !amap.is_marked(d1, mark_modified_darts) )
        {
          d2 = (*it)->beta(i-1)->beta(i);
          while ( d2!=Map::null_dart_handle && amap.is_marked(d2, mark) )
          {
            d2 = d2->beta(i-1)->beta(i);
            if ( d2==(*it)->beta(i-1)->beta(i) ) d2=Map::null_dart_handle;
          }

          if ( !amap.is_marked(d2, mark_modified_darts) )
          {
            if (d1 != Map::null_dart_handle)
            {
              if (d2 != Map::null_dart_handle && d1!=d2 )
              {
                amap.template basic_link_beta<i>(d1, d2);
                amap.mark(d1, mark_modified_darts);
                amap.mark(d2, mark_modified_darts);
                modified_darts.push_back(d1);
                modified_darts.push_back(d2);
              }
              else
              {
                if ( !d1->is_free(i) )
                {
                  d1->unlink_beta(i);
                  CGAL_assertion( !amap.is_marked(d1, mark_modified_darts) );
                  amap.mark(d1, mark_modified_darts);
                  modified_darts.push_back(d1);
                }
              }
            }
            else if (d2 != Map::null_dart_handle)
            {
              if ( !d2->is_free(i) )
              {
                d2->unlink_beta(i);
                CGAL_assertion( !amap.is_marked(d2, mark_modified_darts) );
                amap.mark(d2, mark_modified_darts);
                modified_darts.push_back(d2);
              }
            }
          }
        }
        if ((*it)->is_free(i-1) && !(*it)->is_free(i))
        {
          d1 = (*it)->beta(i);
          if ( !d1->is_free(i) )
          {
            d1->unlink_beta(i);
            CGAL_assertion( !amap.is_marked(d1, mark_modified_darts) );
            amap.mark(d1, mark_modified_darts);
            modified_darts.push_back(d1);
          }
        }
      }

      // We test the split of all the incident cells for all the non
      // void attributes.
      Map::Helper::template Foreach_enabled_attributes
          <internal::Test2_split_with_deque<Map,i> >::
          run(&amap, &modified_darts, mark_modified_darts);

      // We remove all the darts of the i-cell.
      for (  it=to_erase.begin(); it!=to_erase.end(); ++it )
      { amap.erase_dart(*it); }

      CGAL_assertion( amap.is_whole_map_unmarked(mark) );
      amap.free_mark(mark);

      if ( !amap.is_whole_map_unmarked(mark_modified_darts) )
      {
        for ( typename std::deque<typename Map::Dart_handle>::
              iterator it=modified_darts.begin();
              it!=modified_darts.end(); ++it )
          amap.unmark(*it, mark_modified_darts);
      }

      // amap.display_darts(std::cout);

      CGAL_assertion ( amap.is_whole_map_unmarked(mark_modified_darts) );
      amap.free_mark(mark_modified_darts);

      CGAL_expensive_postcondition( amap.is_valid() );
      assert( amap.is_valid() );

      return res;
    }
  };

  /** Contract an edge, and merge eventually both incident vertices.
   * @param amap the used combinatorial map.
   * @param adart a dart of the edge to contract.
   * @return the number of deleted darts.
   */
  template<class Map>
  struct Contract_cell_functor<Map,1>
  {
    static size_t run(Map& amap, typename Map::Dart_handle adart)
    {
      CGAL_assertion( (is_contractible<Map,1>(amap,adart)) );

      size_t res = 0;

      typename Map::Dart_handle d1, d2;
      typename Map::Dart_handle dg1=NULL, dg2=NULL;

      int mark = amap.get_new_mark();
//      int mark_modified_darts = amap.get_new_mark();

      std::deque<typename Map::Dart_handle> to_erase;

      std::deque<typename Map::Dart_handle> modified_darts;

      // First we store and mark all the darts of the 1-cell to contract.
      for ( CMap_dart_iterator_basic_of_cell<Map,1> it(amap,adart,mark);
            it.cont(); ++it )
      {
        to_erase.push_back(it);
        if ( dg1==NULL && it->other_extremity()!=NULL )
        { dg1=it; dg2=it->other_extremity(); }
        amap.mark(it, mark);
        ++res;
      }

      typename std::deque<typename Map::Dart_handle>::iterator it =
        to_erase.begin();

      for (; it != to_erase.end(); ++it)
        amap.update_dart_of_all_attributes(*it, mark);

      // We group the two vertices incident if they exist.
      if ( dg1!=NULL )
        amap.template group_attribute<0>(dg1, dg2);

      // 3) We modify the darts of the cells incident to the edge
      //    when they are marked to remove.
      for (it=to_erase.begin(); it!=to_erase.end(); ++it)
      { amap.update_dart_of_all_attributes(*it, mark); }

      // 4) For each dart of the cell, we modify link of neighbors.
      for ( it=to_erase.begin(); it!=to_erase.end(); ++it )
      {
        if ( !(*it)->is_free(0) )
        {
          if ( !(*it)->is_free(1) && (*it)->beta(0)!=(*it) )
          {
            amap.template basic_link_beta<1>((*it)->beta(0), (*it)->beta(1));
            modified_darts.push_back((*it)->beta(0));
            modified_darts.push_back((*it)->beta(1));

          }
          else
          {
            // TODO todegroup.push(Dart_pair((*it)->beta(0), *it));
            (*it)->beta(0)->unlink_beta(1);
            modified_darts.push_back((*it)->beta(0));
          }
        }
        else
        {
          if ( !(*it)->is_free(1) )
          {
            // TODO todegroup.push(Dart_pair((*it)->beta(1), *it));
            (*it)->beta(1)->unlink_beta(0);
            modified_darts.push_back((*it)->beta(1));
          }
        }
      }

      // We test the split of all the incident cells for all the non
      // void attributes.
      Map::Helper::template Foreach_enabled_attributes
          <internal::Test2_split_with_deque<Map,0> >::
               // <internal::Test_split_with_deque<Map,0> >::
                run(&amap,
                    &modified_darts, -1); //mark_modified_darts);
                    //&mark_for_incident_cells[0],
                    //&incident_cells[0]);

      // 6) We remove all the darts of the cell.
      for (it = to_erase.begin(); it != to_erase.end(); ++it)
      { amap.erase_dart(*it); }

      CGAL_assertion( amap.is_whole_map_unmarked(mark) );
      amap.free_mark(mark);

      CGAL_expensive_postcondition( amap.is_valid() );
      assert( amap.is_valid() ); // TO REMOVE

      return res;
    }
  };

  /** Contract an i-cell, 1<=i<=dimension.
   * @param amap the used combinatorial map.
   * @param adart a dart of the i-cell to remove.
   * @return the number of deleted darts.
   */
  template < class Map, unsigned int i >
  size_t contract_cell(Map& amap, typename Map::Dart_handle adart)
  { return Contract_cell_functor<Map,i>::run(amap,adart); }

  /** Test if a 2-cell can be inserted onto a given 3-cell along
   * a path of edges.
   * @param amap the used combinatorial map.
   * @param afirst iterator on the begining of the path.
   * @param alast  iterator on the end of the path.
   * @return true iff a 2-cell can be inserted along the path.
   */
  template <class Map, class InputIterator>
  bool is_insertable_cell_2_in_cell_3(const Map& amap,
                                      InputIterator afirst,
                                      InputIterator alast)
  {
    CGAL_static_assertion( Map::dimension>= 3 );

    // The path must have at least one dart.
    if (afirst==alast) return false;
    typename Map::Dart_const_handle prec = NULL;
    typename Map::Dart_const_handle od = NULL;

    for (InputIterator it(afirst); it!=alast; ++it)
    {
      // The path must contain only non empty darts.
      if (*it == NULL || *it==Map::null_dart_handle) return false;

      // Two consecutive darts of the path must belong to two edges
      // incident to the same vertex of the same volume.
      if (prec != NULL)
      {
        od = prec->other_extremity();
        if ( od==Map::null_dart_handle ) return false;

        // of and *it must belong to the same vertex of the same volume
        if ( !belong_to_same_cell<Map, 0, 2>(amap, od, *it) )
          return false;
      }
      prec = *it;
    }

    // The path must be closed.
    od = prec->other_extremity();
    if ( od==Map::null_dart_handle ) return false;

    if (!belong_to_same_cell<Map, 0, 2>(amap, od, *afirst))
      return false;

    return true;
  }

  /** Insert a vertex in a given edge.
   * @param amap the used combinatorial map.
   * @param adart a dart of the edge (!=NULL && !=null_dart_handle).
   * @return a dart of the new vertex.
   */
  template<class Map>
  typename Map::Dart_handle
  insert_cell_0_in_cell_1(Map& amap, typename Map::Dart_handle adart)
  {
    CGAL_assertion(adart != NULL && adart!=Map::null_dart_handle);

    typename Map::Dart_handle d1, d2;
    int mark = amap.get_new_mark();

    std::vector<typename Map::Dart_handle> vect;
    {
      for (typename Map::template Dart_of_cell_range<1>::iterator it=
             amap.template darts_of_cell<1>(adart).begin();
           it != amap.template darts_of_cell<1>(adart).end(); ++it)
        vect.push_back(it);
    }

    // 3) For each dart of the cell, we modify link of neighbors.
    typename std::vector<typename Map::Dart_handle>::iterator it = vect.begin();
    for (; it != vect.end(); ++it)
    {
      d1 = amap.create_dart();

      if (!(*it)->is_free(1))
      { amap.template basic_link_beta<1>(d1, (*it)->beta(1)); }

      for ( unsigned int dim = 2; dim<=Map::dimension; ++dim )
      {
        if (!(*it)->is_free(dim) && amap.is_marked((*it)->beta(dim), mark))
        {
          amap.basic_link_beta((*it)->beta(dim), d1, dim);
          amap.basic_link_beta(*it, (*it)->beta(dim)->beta(1), dim);
        }
      }

      amap.template basic_link_beta<1>(*it, d1);
      amap.group_all_dart_attributes_except(*it, d1, 1);

      amap.mark(*it, mark);
    }

    for (it = vect.begin(); it != vect.end(); ++it)
    {  amap.unmark(*it, mark); }

    amap.free_mark(mark);

    amap.template degroup_attribute<1>(adart, adart->beta(1));

    // CGAL_expensive_postcondition( amap.is_valid() );

    return adart->beta(1);
  }

  /** Insert a dangling edge in a 2-cell between given by a dart.
   * @param amap the used combinatorial map.
   * @param adart1 a first dart of the facet (!=NULL && !=null_dart_handle).
   * @return a dart of the new edge, not incident to the vertex of adart1.
   */
  template<class Map>
  typename Map::Dart_handle
  insert_dangling_cell_1_in_cell_2(Map& amap, typename Map::Dart_handle adart1)
  {
    CGAL_assertion(adart1!=NULL && adart1!=Map::null_dart_handle);

    int mark1 = amap.get_new_mark();
    std::vector<typename Map::Dart_handle> to_unmark;
    {
      for ( CMap_dart_iterator_basic_of_cell<Map,0> it(amap,adart1,mark1);
            it.cont(); ++it )
      {
        to_unmark.push_back(it);
        amap.mark(it,mark1);
      }
    }

    typename Map::Dart_handle d1 = NULL;
    typename Map::Dart_handle d2 = NULL;
    unsigned int s1 = 0;

    int treated = amap.get_new_mark();

    CGAL::CMap_dart_iterator_of_involution<Map,1> it1(amap,adart1);

    for ( ; it1.cont(); ++it1)
    {
      d1 = amap.create_dart();
      d2 = amap.create_dart();

      if ( amap.is_marked(it1, mark1) ) s1 = 0;
      else s1 = 1;

      if ( !it1->is_free(s1) )
      {
        if ( s1==0 ) amap.template link_beta<1>(it1->beta(0), d2);
        else amap.template link_beta<0>(it1->beta(1), d2);
      }

      if (s1==0)
      {
        amap.template link_beta<0>(it1, d1);
        amap.template basic_link_beta<0>(d1,d2);
      }
      else
      {
        amap.template link_beta<1>(it1, d1);
        amap.template basic_link_beta<1>(d1,d2);
      }

      amap.link_beta_for_involution(d1, d2, 2);

      for ( unsigned int dim=3; dim<=Map::dimension; ++dim)
      {
        if ( !it1->is_free(dim) &&
             amap.is_marked(it1->beta(dim), treated) )
        {
          amap.basic_link_beta_for_involution(it1->beta(dim)->beta_inv(s1), d1,
                                              dim);
          amap.basic_link_beta_for_involution
            (it1->beta(dim)->beta_inv(s1)->beta(2), d2, dim);
        }
      }

      amap.mark(it1,treated);
    }

    //    amap.template degroup_attribute<1>(adart1, adart1->beta(0));
    //    amap.template degroup_attribute<2>(d1, d2);

    for ( it1.rewind(); it1.cont(); ++it1 )
    {
      amap.unmark(it1,treated);
    }
    CGAL_assertion( amap.is_whole_map_unmarked(treated) );
    amap.free_mark(treated);

    typename std::vector<typename Map::Dart_handle>::iterator it =
      to_unmark.begin();
    for (; it != to_unmark.end(); ++it)
    { amap.unmark(*it, mark1); }
    CGAL_assertion( amap.is_whole_map_unmarked(mark1) );
    amap.free_mark(mark1);

    // CGAL_expensive_postcondition( amap.is_valid() );

    return adart1->beta(0);
  }

  /** Insert an edge in a 2-cell between two given darts.
   * @param amap the used combinatorial map.
   * @param adart1 a first dart of the facet (!=NULL && !=null_dart_handle).
   * @param adart2 a second dart of the facet. If NULL insert a dangling edge.
   * @return a dart of the new edge, and not incident to the
   *         same vertex than adart1.
   */
  template<class CMap>
  typename CMap::Dart_handle
  insert_cell_1_in_cell_2(CMap& amap,
                          typename CMap::Dart_handle adart1,
                          typename CMap::Dart_handle adart2)
  {
    if ( adart2==NULL ) return insert_dangling_cell_1_in_cell_2(amap,adart1);

    CGAL_assertion(is_insertable_cell_1_in_cell_2<CMap>(amap, adart1, adart2));

    int m1 = amap.get_new_mark();
    CMap_dart_iterator_basic_of_involution<CMap,1>
      it1 = CMap_dart_iterator_basic_of_involution<CMap,1>(amap, adart1, m1);
    int m2 = amap.get_new_mark();
    CMap_dart_iterator_basic_of_involution<CMap,1>
      it2 = CMap_dart_iterator_basic_of_involution<CMap,1>(amap, adart2, m2);

    int mark1 = amap.get_new_mark();
    std::vector<typename CMap::Dart_handle> to_unmark;
    {
      for ( CMap_dart_iterator_basic_of_cell<CMap,0> it(amap,adart1,mark1);
            it.cont(); ++it )
      {
        to_unmark.push_back(it);
        amap.mark(it,mark1);
      }
    }

    typename CMap::Dart_handle d1 = NULL;
    typename CMap::Dart_handle d2 = NULL;
    unsigned int s1 = 0;

    int treated = amap.get_new_mark();

    for ( ; it1.cont(); ++it1, ++it2)
    {
      CGAL_assertion (it2.cont() );
      d1 = amap.create_dart();
      d2 = amap.create_dart();

      if ( amap.is_marked(it1, mark1) ) s1 = 0;
      else s1 = 1;

      if ( !it1->is_free(s1) )
      {
        if ( s1==0 ) amap.basic_link_beta_1(it1->beta(0), d2);
        else amap.link_beta_0(it1->beta(1), d2);
      }

      if ( !it2->is_free(s1) )
      {
        if ( s1==0 ) amap.basic_link_beta_1(it2->beta(0), d1);
        else amap.link_beta_0(it2->beta(1), d1);
      }

      if ( s1==0 )
      {
        amap.link_beta_0(it1, d1);
        amap.link_beta_0(it2, d2);
      }
      else
      {
        amap.basic_link_beta_1(it1, d1);
        amap.basic_link_beta_1(it2, d2);
      }
      amap.link_beta_for_involution(d2, d1, 2);

      for ( unsigned int dim=3; dim<=CMap::dimension; ++dim)
      {
        if ( !it1->is_free(dim) &&
             amap.is_marked(it1->beta(dim), treated) )
        {
          amap.basic_link_beta_for_involution
            (it1->beta(dim)->beta_inv(s1), d1, dim);
          amap.basic_link_beta_for_involution
            (it1->beta(dim)->beta_inv(s1)->beta(2), d2, dim);
        }
      }

      amap.mark(it1,treated);
    }

    //    amap.template degroup_attribute<1>(adart1, adart1->beta(0));
    amap.template degroup_attribute<2>(d1, d2);

    amap.negate_mark(m1);
    amap.negate_mark(m2);
    it1.rewind(); it2.rewind();
    for ( ; it1.cont(); ++it1, ++it2)
    {
      amap.mark(it1,m1);
      amap.unmark(it1,treated);
      amap.mark(it2,m2);
    }
    amap.negate_mark(m1);
    amap.negate_mark(m2);
    CGAL_assertion( amap.is_whole_map_unmarked(m1) );
    CGAL_assertion( amap.is_whole_map_unmarked(m2) );
    CGAL_assertion( amap.is_whole_map_unmarked(treated) );
    amap.free_mark(m1);
    amap.free_mark(m2);
    amap.free_mark(treated);

    typename std::vector<typename CMap::Dart_handle>::iterator it =
      to_unmark.begin();
    for (; it != to_unmark.end(); ++it)
    { amap.unmark(*it, mark1); }
    CGAL_assertion( amap.is_whole_map_unmarked(mark1) );
    amap.free_mark(mark1);

    // CGAL_expensive_postcondition( amap.is_valid() );

    return adart1->beta(0);
  }

  /** Insert a 2-cell in a given 3-cell along a path of darts.
   * @param amap the used combinatorial map.
   * @param afirst iterator on the begining of the path.
   * @param alast  iterator on the end of the path.
   * @return a dart of the new 2-cell.
   */
  template<class Map, class InputIterator>
  typename Map::Dart_handle
  insert_cell_2_in_cell_3(Map& amap, InputIterator afirst, InputIterator alast)
  {
    CGAL_assertion(is_insertable_cell_2_in_cell_3(amap,afirst,alast));

    typename Map::Dart_handle prec = NULL, d = NULL, dd = NULL, first = NULL;
    bool withBeta3 = false;

    {
      for (InputIterator it(afirst); it!=alast; ++it)
      {
        if (!(*it)->is_free(2)) withBeta3 = true;
      }
    }

    {
      for (InputIterator it(afirst); it!=alast; ++it)
      {
        d = amap.create_dart();
        if (withBeta3)
        {
          dd = amap.create_dart();
          amap.basic_link_beta_for_involution(d, dd, 3);
        }

        if (prec != NULL)
        {
          amap.template link_beta<0>(prec, d);
          if (withBeta3) amap.template link_beta<1>(prec->beta(3), dd);
        }
        else first = d;

        if (!(*it)->is_free(2))
          amap.link_beta_for_involution((*it)->beta(2), dd, 2);

        amap.link_beta_for_involution(*it, d, 2);

        prec = d;
      }
    }

    amap.template link_beta<0>(prec, first);
    if (withBeta3)
    {
      amap.template link_beta<1>(prec->beta(3), first->beta(3));
    }

    // Make copies of the new facet for dimension >=4
    for ( unsigned int dim=4; dim<=Map::dimension; ++dim )
    {
      if ( !first->is_free(dim) )
      {
        typename Map::Dart_handle first2 = NULL;
        prec = NULL;
        for ( CMap_dart_iterator_of_orbit<Map,1> it(amap, first);
              it.cont(); ++it )
        {
          d = amap.create_dart();
          amap.link_beta_for_involution(it->beta(2),d,dim);
          if ( withBeta3 )
          {
            dd = amap.create_dart();
            amap.link_beta_for_involution(it->beta(2)->beta(3),dd,dim);
            amap.basic_link_beta_for_involution(d, dd, 3);
          }
          if ( prec!=NULL )
          {
            amap.link_beta_0(prec,d);
            if ( withBeta3 )
            {
              amap.link_beta_1(prec->beta(3),dd);
            }
          }
          else first2 = prec;

          for ( unsigned dim2=2; dim2<=Map::dimension; ++dim2 )
          {
            if ( dim2+1!=dim && dim2!=dim && dim2!=dim+1 )
            {
              if ( !it->is_free(dim2) &&
                   it->beta(dim2)->is_free(dim) )
                amap.basic_link_beta_for_involution(it->beta(dim2)->beta(dim),
                                                    d, dim2);
              if ( withBeta3 && !it->beta(3)->is_free(dim2) &&
                   it->beta(3)->beta(dim2)->is_free(dim) )
                amap.basic_link_beta_for_involution
                  (it->beta(3)->beta(dim2)->beta(dim), dd, dim2);
            }
          }
          prec = d;
        }
        amap.template link_beta<0>( prec, first2 );
        if ( withBeta3 )
        {
          amap.template link_beta<1>( prec->beta(3), first2->beta(3) );
        }
      }
    }

    // Degroup the attributes
    if ( withBeta3 )
      amap.template degroup_attribute<3>( first, first->beta(3) );

    // CGAL_expensive_postcondition( amap.is_valid() );

    return first;
  }
} // namespace CGAL

#endif // CGAL_COMBINATORIAL_MAP_OPERATIONS_H //
// EOF //
